#include <iostream>
#include <vector>

#include "CGL/vector2D.h"

#include "mass.h"
#include "rope.h"
#include "spring.h"

namespace CGL {

    Rope::Rope(Vector2D start, Vector2D end, int num_nodes, float node_mass, float k, vector<int> pinned_nodes)
    {
        // TODO (Part 1): Create a rope starting at `start`, ending at `end`, and containing `num_nodes` nodes.
        /*
         * 这里只给出了绳子开头质点的位置start和绳子结尾质点的位置end，没有给出中间质点的位置
         * 这里就做一个假设，假设中间的质点初始状态下在start和end连接的直线上，且质点之间的距离是相等的
         */
        float rest_len = (end - start).norm() / (num_nodes - 1);   // 每个质点之间的距离，即弹簧的自然长度
        Vector2D dir = (end - start) / (end - start).norm();    // 单位向量
        for (int i = 0; i < num_nodes; ++i)
        {
            masses.push_back(new Mass(start + i * rest_len * dir, node_mass, false));
        }

//        Comment-in this part when you implement the constructor
//        for (auto &i : pinned_nodes) {
//            masses[i]->pinned = true;
//        }
       for (auto &i : pinned_nodes) 
       {
           masses[i]->pinned = true;
       }
       // 弹簧列表的初始化
       for (int i = 0; i < num_nodes - 1; ++i)
       {
           springs.push_back(new Spring(masses[i], masses[i+1], k));
       }
    }

    Rope::~Rope() 
    {
        for (auto* m : masses)
        {
            delete m;
        }
        for (auto* s : springs)
        {
            delete s;
        }
    }

    void Rope::simulateEuler(float delta_t, Vector2D gravity)
    {
        Vector2D ab;     // m1对应a，m2对应b
        Vector2D fba;    // 这个变量表示m2受到的力
        Vector2D fa;     // m1受到的damping force
        Vector2D fb;     // m2受到的damping force
        float kd = 0.1;
        for (auto &s : springs)
        {
            // TODO (Part 2): Use Hooke's law to calculate the force on a node
            ab = s->m2->position - s->m1->position;
            fba =  -s->k * ab.unit() * (ab.norm() - s->rest_length); 
            s->m2->forces += fba;     // 不能直接赋值因为质点受到了两个弹簧的作用
            s->m1->forces += -fba;    // 算一次胡克定律即可

            // 计算damping force
            fb = -kd * dot(ab.unit(), s->m2->velocity - s->m1->velocity) * ab.unit();
            fa = -kd * dot(-ab.unit(), s->m1->velocity - s->m2->velocity) * (-ab.unit());

            s->m2->forces += fb;
            s->m1->forces += fa;
        }

        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                // TODO (Part 2): Add the force due to gravity, then compute the new velocity and position
                m->forces += gravity * m->mass;    // 都是向量的坐标直接加
                // TODO (Part 2): Add global damping


                // 半隐式，利用的是更新后的速度
                m->velocity = m->velocity + m->forces / m->mass * delta_t;
                m->position = m->position + m->velocity * delta_t;

                /* 
                显示欧拉，利用上一个时刻的速度
                m->position = m->position + m->velocity * delta_t;
                m->velocity = m->velocity + m->forces / m->mass * delta_t;
                */
            }

            // Reset all forces on each mass
            m->forces = Vector2D(0, 0);
        }
    }


    void Rope::simulateVerlet(float delta_t, Vector2D gravity)
    {
        Vector2D ab;     // m1对应a，m2对应b
        Vector2D fba;    // 这个变量表示m2受到的力
        for (auto &s : springs)
        {
            // TODO (Part 3): Simulate one timestep of the rope using explicit Verlet （solving constraints)
            // TODO (Part 2): Use Hooke's law to calculate the force on a node
            ab = s->m2->position - s->m1->position;
            fba =  -s->k * ab.unit() * (ab.norm() - s->rest_length); 
            s->m2->forces += fba;     // 不能直接赋值因为质点受到了两个弹簧的作用
            s->m1->forces += -fba;    // 算一次胡克定律即可
        }

        float damping_factor = 0.00005;
        for (auto &m : masses)
        {
            if (!m->pinned)
            {
                Vector2D temp_position = m->position;
                // TODO (Part 3.1): Set the new position of the rope mass
                
                m->forces += gravity * m->mass;
                // TODO (Part 4): Add global Verlet damping
                m->position = m->position + (1 - damping_factor) * (m->position - m->last_position)
                              + m->forces / m->mass  * delta_t * delta_t;

               m->last_position = temp_position;
            }

            m->forces = Vector2D(0, 0);
        }
    }
}
