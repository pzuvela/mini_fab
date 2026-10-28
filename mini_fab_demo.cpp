#include <vector>
#include <memory>

#include "irobot.h"
#include "sim_robot.h"
#include "mock_robot.h"
#include "process_engine.h"
#include "sim_process_module.h"


void runRobot(IRobot& robot, IProcessModule& processModule) 
{
    ProcessEngine engine(robot, processModule);

    Wafer wafer
    {
        "5",
        Location{ Station::Aligner, 3 }
    };

    Location destination
    {
        Station::LoadPort,
        4
    };

    engine.run(wafer, destination);
}

int main()
{
    std::vector<std::unique_ptr<IRobot>> robots;

    robots.push_back(std::make_unique<SimRobot>());
    robots.push_back(std::make_unique<MockRobot>(true));
    robots.push_back(std::make_unique<MockRobot>(false));

    SimProcessModule processModule;
    processModule.initialize();

    for (const std::unique_ptr<IRobot>& robot : robots) 
    {
        runRobot(*robot, processModule);
    }

    return 0;
};
