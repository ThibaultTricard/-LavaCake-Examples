#include <LavaCake/Framework/Framework.h>
using namespace LavaCake::Framework;

#define dataSize 4096


#ifdef __APPLE__
std::string prefix = "../";
#else
std::string prefix = "";
#endif

int main() {
  Device* d = Device::getDevice();
  d->initDevices(1, 0);

  ComputeQueue computeQueue = d->getComputeQueue(0);
  CommandBuffer commandBuffer;

  std::vector<float> A(dataSize);
  std::vector<float> B(dataSize);
  for (int i = 0; i < dataSize; i++) {
    A[i] = i;
    B[i] = i * 2;
  }

  Buffer ABuffer(computeQueue, commandBuffer, A, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
  Buffer BBuffer(computeQueue, commandBuffer, B, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
  Buffer CBuffer(dataSize * sizeof(float), VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT| VK_BUFFER_USAGE_TRANSFER_SRC_BIT);


  ComputePipeline sumPipeline;

  ComputeShaderModule sumShader(prefix + "Data/Shaders/ArraySum/sum.comp.spv");
  sumPipeline.setComputeModule(sumShader);

  DescriptorSet  descriptorSet;

  descriptorSet.addTexelBuffer(ABuffer, VK_SHADER_STAGE_COMPUTE_BIT, 0);
  descriptorSet.addTexelBuffer(BBuffer, VK_SHADER_STAGE_COMPUTE_BIT, 1);
  descriptorSet.addTexelBuffer(CBuffer, VK_SHADER_STAGE_COMPUTE_BIT, 2);

  descriptorSet.compile();

  sumPipeline.setDescriptorLayout(descriptorSet.getLayout());

  sumPipeline.compile();

  commandBuffer.beginRecord();
  sumPipeline.bindPipeline(commandBuffer);
  sumPipeline.bindDescriptorSet(commandBuffer,descriptorSet);
  sumPipeline.compute(commandBuffer, dataSize / 32, 1, 1);
  commandBuffer.endRecord();
  commandBuffer.submit(computeQueue, {}, {});
  commandBuffer.wait();
  commandBuffer.resetFence();

  std::vector<float> C;
  CBuffer.readBack(computeQueue, commandBuffer, C);

  for (auto c : C) {
    std::cout << c << " ";
  }
  std::cout << std::endl;

  return 0;
}