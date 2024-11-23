#include <cstdio>
#include <NvInfer.h>
using namespace nvinfer1;


struct Logger: public ILogger
{
	public:
		void log(Severity level, char const* info)
		{
			if (level <= Severity::kWARNING)
				fputs(info, stderr);
		}
};


int main()
{
	cudaStream_t stream;
	cudaStreamCreate(&stream);
	Logger logger;
	IRuntime* runtime = createInferRuntime(logger);
	ICudaEngine* engine = runtime->deserializeCudaEngine(nullptr, 0);
	IExecutionContext* context = engine->createExecutionContext();
	cudaEvent_t eStart, eStop;
	cudaEventCreate(&eStart);
	cudaEventCreate(&eStop);
	cudaEventRecord(eStart, stream);
	cudaStreamWaitEvent(stream, eStop);
	context->enqueueV2(nullptr, stream, nullptr);
	float elapse;
	cudaEventRecord(eStop, stream);
	cudaEventElapsedTime(&elapse, eStart, eStop);
	delete context;
	delete engine;
	delete runtime;
	delete stream;
}
