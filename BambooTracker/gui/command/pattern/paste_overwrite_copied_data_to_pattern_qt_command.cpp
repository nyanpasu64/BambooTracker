#include "paste_overwrite_copied_data_to_pattern_qt_command.hpp"

PasteOverwriteCopiedDataToPatternQtCommand::PasteOverwriteCopiedDataToPatternQtCommand(PatternEditorPanel* panel, QUndoCommand* parent)
	: QUndoCommand(parent),
	  panel_(panel)
{
}

void PasteOverwriteCopiedDataToPatternQtCommand::redo()
{
	panel_->update();
}

void PasteOverwriteCopiedDataToPatternQtCommand::undo()
{
	panel_->update();
}

int PasteOverwriteCopiedDataToPatternQtCommand::id() const
{
	return 0x3a;
}
