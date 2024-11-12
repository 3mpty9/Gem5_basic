#include "cpu/minor/dummyexecute.hh"

#include "base/logging.hh"
#include "base/trace.hh"
#include "cpu/minor/pipeline.hh"
//#include "debug/dummyexecute.hh

namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(Minor, minor);
namespace minor
{

DummyExecute::DummyExecute(const std::string &name,
    MinorCPU&cpu_,
    const BaseMinorCPUParams &params,
    Latch<ForwardInstData>::Output inp_,
    Latch<ForwardInstData>::Input out_,
    std::vector<InputBuffer<ForwardInstData>> &next_stage_input_buffer) :
    Named(name),
    cpu(cpu_),
    inp(inp_),
    out(out_),
    nextStageReserve(next_stage_input_buffer),
    outputWidth(params.executeInputWidth),
    processMoreThanOneInput(params.dummyexecuteCycleInput),
    dummyexecuteInfo(params.numThreads),
    threadPriority(0)
{
    if (outputWidth < 1)
            fatal("%s: executeInputWidth must be >= 1 (%d)\n", name, outputWidth);

    if (params.dummyexecuteInputBufferSize < 1) {
        fatal("%s: dummyexecuteInputBufferSize must be >= 1 (%d)\n", name,
        params.dummyexecuteInputBufferSize);
    }

        /* Per-thread input buffers */
    for (ThreadID tid = 0; tid < params.numThreads; tid++) {
        inputBuffer.push_back(
            InputBuffer<ForwardInstData>(
                name + ".inputBuffer" + std::to_string(tid), "insts",
                params.dummyexecuteInputBufferSize));
    }
}

const ForwardInstData *
DummyExecute::getInput(ThreadID tid)
{
    /* Get insts from the inputBuffer to work with */
    if (!inputBuffer[tid].empty()) {
        const ForwardInstData &head = inputBuffer[tid].front();

        return (head.isBubble() ? NULL : &(inputBuffer[tid].front()));
    } else {
        return NULL;
    }
}

void
DummyExecute::popInput(ThreadID tid)
{
    if (!inputBuffer[tid].empty())
        inputBuffer[tid].pop();

    dummyexecuteInfo[tid].inputIndex = 0;
    dummyexecuteInfo[tid].inMacroop = false;
}

#if TRACING_ON
/** Add the tracing data to an instruction.  This originates in
 *  decode because this is the first place that execSeqNums are known
 *  (these are used as the 'FetchSeq' in tracing data) */
static void
dynInstAddTracing(MinorDynInstPtr inst, StaticInstPtr static_inst,
    MinorCPU &cpu)
{
    inst->traceData = cpu.getTracer()->getInstRecord(curTick(),
        cpu.getContext(inst->id.threadId),
        inst->staticInst, *inst->pc, static_inst);

    /* Use the execSeqNum as the fetch sequence number as this most closely
     *  matches the other processor models' idea of fetch sequence */
    if (inst->traceData)
        inst->traceData->setFetchSeq(inst->id.execSeqNum);
}
#endif

void
DummyExecute::evaluate()
{
    /* Push input onto appropriate input buffer */
    if (!inp.outputWire->isBubble())
        inputBuffer[inp.outputWire->threadId].setTail(*inp.outputWire);

    ForwardInstData &insts_out = *out.inputWire;

    assert(insts_out.isBubble());

    for (ThreadID tid = 0; tid < cpu.numThreads; tid++)
        dummyexecuteInfo[tid].blocked = !nextStageReserve[tid].canReserve();

    ThreadID tid = getScheduledThread();

    if (tid != InvalidThreadID) {
        DummyExecuteThreadInfo &dummyexecute_info = dummyexecuteInfo[tid];
        const ForwardInstData *insts_in = getInput(tid);

        unsigned int output_index = 0;

        /* Pack instructions into the output while we can.  This may involve
         * using more than one input line */
        while (insts_in &&
           dummyexecute_info.inputIndex < insts_in->width() && /* Still more input */
           output_index < outputWidth /* Still more output to fill */)
        {
            MinorDynInstPtr inst = insts_in->insts[dummyexecute_info.inputIndex];

            if (inst->isBubble()) {
                /* Skip */
                dummyexecute_info.inputIndex++;
                dummyexecute_info.inMacroop = false;
            } else {
                StaticInstPtr static_inst = inst->staticInst;
                /* Static inst of a macro-op above the output_inst */
                StaticInstPtr parent_static_inst = NULL;
                MinorDynInstPtr output_inst = inst;

                if (inst->isFault()) {
                    //DPRINTF(DummyExecute, "Fault being passed: %d\n",
                    //    inst->fault->name());

                    dummyexecute_info.inputIndex++;
                    dummyexecute_info.inMacroop = false;
                }  else {
                    /* Doesn't need decomposing, pass on instruction */
                    //DPRINTF(DummyExecute, "Passing on inst: %s inputIndex:"
                    //    " %d output_index: %d\n",
                    //    *output_inst, dummyexecute_info.inputIndex, output_index);

                    parent_static_inst = static_inst;

                    /* Step input */
                    dummyexecute_info.inputIndex++;
                    dummyexecute_info.inMacroop = false;
                }

                /* Set execSeqNum of output_inst */
                output_inst->id.execSeqNum = dummyexecute_info.execSeqNum;
                /* Add tracing */
#if TRACING_ON
                dynInstAddTracing(output_inst, parent_static_inst, cpu);
#endif

                /* Step to next sequence number */
                dummyexecute_info.execSeqNum++;

                /* Correctly size the output before writing */
                if (output_index == 0) insts_out.resize(outputWidth);
                /* Push into output */
                insts_out.insts[output_index] = output_inst;
                output_index++;
            }

            /* Have we finished with the input? */
            if (dummyexecute_info.inputIndex == insts_in->width()) {
                /* If we have just been producing micro-ops, we *must* have
                 * got to the end of that for inputIndex to be pushed past
                 * insts_in->width() */
                assert(!dummyexecute_info.inMacroop);
                popInput(tid);
                insts_in = NULL;

                if (processMoreThanOneInput) {
                    //DPRINTF(DummyExecute, "Wrapping\n");
                    insts_in = getInput(tid);
                }
            }
        }

        /* The rest of the output (if any) should already have been packed
         *  with bubble instructions by insts_out's initialisation
         *
         *  for (; output_index < outputWidth; output_index++)
         *      assert(insts_out.insts[output_index]->isBubble());
         */
    }

    /* If we generated output, reserve space for the result in the next stage
     *  and mark the stage as being active this cycle */
    if (!insts_out.isBubble()) {
        /* Note activity of following buffer */
        cpu.activityRecorder->activity();
        insts_out.threadId = tid;
        nextStageReserve[tid].reserve();
    }

    /* If we still have input to process and somewhere to put it,
     *  mark stage as active */
    for (ThreadID i = 0; i < cpu.numThreads; i++)
    {
        if (getInput(i) && nextStageReserve[i].canReserve()) {
            cpu.activityRecorder->activateStage(Pipeline::DummyExecuteStageId);
            break;
        }
    }

    /* Make sure the input (if any left) is pushed */
    if (!inp.outputWire->isBubble())
        inputBuffer[inp.outputWire->threadId].pushTail();
}

inline ThreadID
DummyExecute::getScheduledThread()
{
    /* Select thread via policy. */
    std::vector<ThreadID> priority_list;

    switch (cpu.threadPolicy) {
      case enums::SingleThreaded:
        priority_list.push_back(0);
        break;
      case enums::RoundRobin:
        priority_list = cpu.roundRobinPriority(threadPriority);
        break;
      case enums::Random:
        priority_list = cpu.randomPriority();
        break;
      default:
        panic("Unknown fetch policy");
    }

    for (auto tid : priority_list) {
        if (getInput(tid) && !dummyexecuteInfo[tid].blocked) {
            threadPriority = tid;
            return tid;
        }
    }

   return InvalidThreadID;
}

bool
DummyExecute::isDrained()
{
    for (const auto &buffer : inputBuffer) {
        if (!buffer.empty())
            return false;
    }

    return (*inp.outputWire).isBubble();
}

void
DummyExecute::minorTrace() const
{
    std::ostringstream data;

    if (dummyexecuteInfo[0].blocked)
        data << 'B';
    else
        (*out.inputWire).reportData(data);

    minor::minorTrace("insts=%s\n", data.str());
    inputBuffer[0].minorTrace();
}

}//namespace minor

}//namespace gem5