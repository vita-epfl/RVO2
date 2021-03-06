/*
 * Blocks.cpp
 * RVO2 Library
 *
 * Copyright 2008 University of North Carolina at Chapel Hill
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please send all bug reports to <geom@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Jur van den Berg, Stephen J. Guy, Jamie Snape, Ming C. Lin, Dinesh Manocha
 * Dept. of Computer Science
 * 201 S. Columbia St.
 * Frederick P. Brooks, Jr. Computer Science Bldg.
 * Chapel Hill, N.C. 27599-3175
 * United States of America
 *
 * <http://gamma.cs.unc.edu/RVO2/>
 */

/*
 * Example file showing a demo with 100 agents split in four groups initially
 * positioned in four corners of the environment. Each agent attempts to move to
 * other side of the environment through a narrow passage generated by four
 * obstacles. There is no roadmap to guide the agents around the obstacles.
 */

#ifndef RVO_OUTPUT_TIME_AND_POSITIONS
#define RVO_OUTPUT_TIME_AND_POSITIONS 1
#endif

#ifndef RVO_SEED_RANDOM_NUMBER_GENERATOR
#define RVO_SEED_RANDOM_NUMBER_GENERATOR 1
#endif

#include <cmath>
#include <cstdlib>
#include <sys/stat.h>
#include <fstream>
#include <string>
#include <sstream>

#include <vector>

#if RVO_OUTPUT_TIME_AND_POSITIONS
#include <iostream>
#endif

#if RVO_SEED_RANDOM_NUMBER_GENERATOR
#include <ctime>
#endif

#if _OPENMP
#include <omp.h>
#endif

#include <RVO.h>

#ifndef M_PI
const float M_PI = 3.14159265358979323846f;
#endif

using namespace std;

/* Store the goals of the agents. */
std::vector<RVO::Vector2> goals;


void setupScenario(RVO::RVOSimulator *sim, ofstream& output_file)
{
    float radius = 0.3f;
    float max_speed = 1.0f;

    /* Specify the global time step of the simulation. */
    sim->setTimeStep(1.0f);

    /* Specify the default parameters for agents that are subsequently added. */
    /* neighborDist, maxNeighbors, timeHorizon, timeHorizonObst, radius, maxSpeed, &velocity*/
    sim->setAgentDefaults(15.0f, 10, 5.0f, 5.0f, radius, max_speed);

    float angle1, angle2;
    RVO::Vector2 p1, p2, g1, g2;
    do {
        angle1 = float(rand()) / RAND_MAX * 2 * M_PI;
        angle2 = float(rand()) / RAND_MAX * 2 * M_PI;

        p1 = RVO::Vector2(+cos(angle1)*2, +sin(angle1)*2);
        g1 = RVO::Vector2(-cos(angle1)*2, -sin(angle1)*2);
        p2 = RVO::Vector2(+cos(angle2)*2, +sin(angle2)*2);
        g2 = RVO::Vector2(-cos(angle2)*2, -sin(angle2)*2);

    } while (RVO::absSq(p1 - p2) < 0.2);


    sim->addAgent(p1);
    goals.push_back(g1);
    sim->addAgent(p2);
    goals.push_back(g2);

    // print goal position, radius and preferred speed
    output_file << g1.x() << " " << g1.y() << " " << radius << " " << max_speed << std::endl;
    output_file << g2.x() << " " << g2.y() << " " << radius << " " << max_speed << std::endl;
}

#if RVO_OUTPUT_TIME_AND_POSITIONS
void updateVisualization(RVO::RVOSimulator *sim, ofstream& output_file)
{
    /* Output the current global time. */
    output_file << sim->getGlobalTime();

    /* Output the current position of all the agents. */
    for (size_t i = 0; i < sim->getNumAgents(); ++i) {
        output_file << " " << sim->getAgentPosition(i);
    }

    output_file << std::endl;
}
#endif

void setPreferredVelocities(RVO::RVOSimulator *sim)
{
    /*
     * Set the preferred velocity to be a vector of unit magnitude (speed) in the
     * direction of the goal.
     */
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < static_cast<int>(sim->getNumAgents()); ++i) {
        RVO::Vector2 goalVector = goals[i] - sim->getAgentPosition(i);

        if (RVO::absSq(goalVector) > 1.0f) {
            goalVector = RVO::normalize(goalVector);
        }

        sim->setAgentPrefVelocity(i, goalVector);

        /*
         * Perturb a little to avoid deadlocks due to perfect symmetry.
         */
        float angle = std::rand() * 2.0f * M_PI / RAND_MAX;
        float dist = std::rand() * 0.0001f / RAND_MAX;

        sim->setAgentPrefVelocity(i, sim->getAgentPrefVelocity(i) +
                                     dist * RVO::Vector2(std::cos(angle), std::sin(angle)));
    }
}

bool reachedGoal(RVO::RVOSimulator *sim)
{
    /* Check if all agents have reached their goals. */
    for (size_t i = 0; i < sim->getNumAgents(); ++i) {
        if (RVO::absSq(sim->getAgentPosition(i) - goals[i]) > sim->getAgentRadius(i)*sim->getAgentRadius(i)) {
//            if (i ==0)
//                cout << RVO::absSq(sim->getAgentPosition(i) - goals[i]) << " " << sim->getAgentRadius(i)*sim->getAgentRadius(i) << endl;
            return false;
        }
//        else
//            if (i==0)
//                cout << sim->getAgentPosition(i) << " " << goals[i] << endl;
//                cout << RVO::absSq(sim->getAgentPosition(i) - goals[i]) << " " << sim->getAgentRadius(i)*sim->getAgentRadius(i) << endl;
    }

    return true;
}


int run_simulation_once(ofstream& output_file)
{
    /* Create a new simulator instance. */
    RVO::RVOSimulator *sim = new RVO::RVOSimulator();
    goals.clear();

    /* Set up the scenario. */
    setupScenario(sim, output_file);

    /* Perform (and manipulate) the simulation. */
    do {
#if RVO_OUTPUT_TIME_AND_POSITIONS
        updateVisualization(sim, output_file);
#endif
        setPreferredVelocities(sim);
        sim->doStep();
    }
    while (!reachedGoal(sim));
#if RVO_OUTPUT_TIME_AND_POSITIONS
    updateVisualization(sim, output_file);
#endif

    delete sim;

    return 0;
}


int main()
{
#if RVO_SEED_RANDOM_NUMBER_GENERATOR
    std::srand(static_cast<unsigned int>(std::time(NULL)));
#endif
    mkdir("multi_sim", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    for (int i=0; i < 1500; i++) {
        ofstream output_file;
        ostringstream file_name;
        file_name << "multi_sim/" << i << ".txt";
        cout << "Writing to " << file_name.str() << endl;
        output_file.open(file_name.str().c_str());
        run_simulation_once(output_file);
        output_file.close();
    }
}
