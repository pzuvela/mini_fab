#include <mutex>
#include <condition_variable>

#include <iprocess_module.h>


class BlockingMockProcessModule : public IProcessModule
{
private:
    bool ready_ = false;
    bool processingStarted_ = false;
    bool released_ = false;

    std::mutex mutex_;
    std::condition_variable condition_;

public:
    void initialize() override;
    bool isReady() const override;
    bool process(const Wafer& wafer, const Recipe& recipe) override;

    void waitUntilProcessingStarts();
    void release();
};