#include <vector>
#include <memory>

#include "iwafer_manager.h"
#include "tests/mocks/mock_wafer_manager.h"
#include "irobot.h"
#include "sim_robot.h"
#include "tests/mocks/mock_robot.h"
#include "engine/process_engine.h"
#include "sim_process_module.h"


void runProcessWafer(IWaferManager& mgr, IRobot& robot, IProcessModule& processModule, Recipe& recipe) 
{
    ProcessEngine engine(mgr, robot, processModule);

    Wafer wafer
    {
        "5",
        Location{ Station::Aligner, 3 }
    };

    mgr.addWafer(wafer);

    Location destination
    {
        Station::LoadPort,
        4
    };

    engine.processWafer("5", destination, recipe);
}

int main()
{
    Wafer wafer{ "5", Location{ Station::Aligner, 3 } };

    Recipe recipe{ .id = 1, .name = "Recipe1", .durationSeconds = 50.2 };

    MockWaferManager mgr{ std::make_unique<Wafer>(wafer)};

    std::vector<std::unique_ptr<IRobot>> robots;

    robots.push_back(std::make_unique<SimRobot>());
    robots.push_back(std::make_unique<MockRobot>(true));
    robots.push_back(std::make_unique<MockRobot>(false));

    SimProcessModule processModule;
    processModule.initialize();

    for (const std::unique_ptr<IRobot>& robot : robots) 
    {
        runProcessWafer(mgr, *robot, processModule, recipe);
    }

    return 0;
};
