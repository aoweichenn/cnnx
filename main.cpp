//
// Created by aowei on 25-6-22.
//
#include <iostream>

#include <ir/graph.hpp>

#include "utils/storezip.hpp"

using namespace cnnx;
using namespace std;

int main(int argc, char* argv[])
{
    Graph graph;
    graph.load("./resnet18.param",
               "./resnet18.bin");
    // graph.parse("./resnet18.param");
    const int error = graph.save("./test.param", "./test.bin");
    std::cout << error << std::endl;
}
