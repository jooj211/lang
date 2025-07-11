#ifndef BLOCK_CMD_NODE_HPP
#define BLOCK_CMD_NODE_HPP
#include "Command.hpp"
#include "Visitor.hpp"
#include <vector>
class BlockCmdNode : public Command {
public:
    std::vector<Command*> commands;
    explicit BlockCmdNode(std::vector<Command*>* cmds) { if(cmds) { commands = *cmds; delete cmds; } }
    ~BlockCmdNode() { for (auto cmd : commands) { delete cmd; } }
    void accept(Visitor* v) override { v->visit(this); }
};
#endif