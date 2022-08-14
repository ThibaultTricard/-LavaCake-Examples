#define LAVACAKE_WINDOW_MANAGER_HEADLESS

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
  d->initDevices(1, 1);

  ComputeQueue computeQueue = d->getComputeQueue(0);
  ComputePipeline sumPipeline;
  std::vector<float> A(dataSize);
  std::vector<float> B(dataSize);
  for (int i = 0; i < dataSize; i++) {
    A[i] = i;
    B[i] = i * 2;
  }

  CommandBuffer commandBuffer;
  Buffer ABuffer(computeQueue, commandBuffer, A, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
  Buffer BBuffer(computeQueue, commandBuffer, B, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
  Buffer CBuffer(dataSize * sizeof(float), VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT| VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  std::shared_ptr <DescriptorSet>  descriptorSet = std::make_shared<DescriptorSet>();

  descriptorSet->addTexelBuffer(ABuffer, VK_SHADER_STAGE_COMPUTE_BIT, 0);
  descriptorSet->addTexelBuffer(BBuffer, VK_SHADER_STAGE_COMPUTE_BIT, 1);
  descriptorSet->addTexelBuffer(CBuffer, VK_SHADER_STAGE_COMPUTE_BIT, 2);

  sumPipeline.setDescriptorSet(descriptorSet);

  ComputeShaderModule sumShader(prefix + "Data/Shaders/ArraySum/sum.comp.spv");
  sumPipeline.setComputeModule(sumShader);
  sumPipeline.compile();


  commandBuffer.beginRecord();
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