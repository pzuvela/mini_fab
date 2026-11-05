#include <atomic>
#include <stdexcept>

class FaultInjectingController : public MiniFabController
{
public:
    using MiniFabController::MiniFabController;

    std::atomic<bool> injectFault{true};

protected:
    OperationResult awaitEngineCompletion() override
    {
        const auto result =
            MiniFabController::awaitEngineCompletion();

        if (injectFault.exchange(false))
        {
            throw std::runtime_error(
                "Injected completion failure"
            );
        }

        return result;
    }
};

