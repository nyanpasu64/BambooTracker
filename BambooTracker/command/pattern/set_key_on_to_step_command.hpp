#pragma once

#include "abstract_command.hpp"
#include <memory>
#include "module.hpp"

class SetKeyOnToStepCommand : public AbstractCommand
{
public:
	SetKeyOnToStepCommand(std::weak_ptr<Module> mod, int songNum, int trackNum, int orderNum, int stepNum, int noteNum, int instNum);
	void redo() override;
	void undo() override;
	int getID() const override;

private:
	std::weak_ptr<Module> mod_;
	int song_, track_, order_, step_, note_, inst_;
	int prevNote_, prevInst_;
};
