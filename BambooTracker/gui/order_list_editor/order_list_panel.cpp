#include "order_list_panel.hpp"
#include <QPainter>
#include <QFontMetrics>
#include <QApplication>
#include <QPoint>
#include <algorithm>
#include "gui/event_guard.hpp"
#include "gui/command/order/order_commands.hpp"

#include <QDebug>

OrderListPanel::OrderListPanel(QWidget *parent)
	: QWidget(parent),
	  leftTrackNum_(0),
	  curSongNum_(0),
	  curPos_{ 0, 0 },
	  hovPos_{ -1, -1 },
	  editPos_{ -1, -1 },
	  mousePressPos_{ -1, -1 },
	  mouseReleasePos_{ -1, -1 },
	  selLeftAbovePos_{ -1, -1 },
	  selRightBelowPos_{ -1, -1 },
	  shiftPressedPos_{ -1, -1 },
	  isIgnoreToSlider_(false),
	  isIgnoreToPattern_(false),
	  entryCnt_(0)
{
	/* Font */
	headerFont_ = QApplication::font();
	headerFont_.setPointSize(10);
	rowFont_ = QFont("Monospace", 10);
	rowFont_.setStyleHint(QFont::TypeWriter);
	rowFont_.setStyleStrategy(QFont::ForceIntegerMetrics);
	// Check font size
	QFontMetrics metrics(rowFont_);
	rowFontWidth_ = metrics.width('0');
	rowFontAscend_ = metrics.ascent();
	rowFontHeight_ = metrics.height();
	rowFontLeading_ = metrics.leading();

	/* Width & height */
	widthSpace_ = rowFontWidth_ / 2;
	rowNumWidth_ = rowFontWidth_ * 2 + widthSpace_;
	trackWidth_ = rowFontWidth_ * 5;
	headerHeight_ = rowFontHeight_;

	/* Color */
	defTextColor_ = QColor::fromRgb(180, 180, 180);
	defRowColor_ = QColor::fromRgb(0, 0, 40);
	curTextColor_ = QColor::fromRgb(255, 255, 255);
	curRowColor_ = QColor::fromRgb(110, 90, 140);
	curRowColorEditable_ = QColor::fromRgb(140, 90, 110);
	curCellColor_ = QColor::fromRgb(255, 255, 255, 127);
	selCellColor_ = QColor::fromRgb(100, 100, 200, 192);
	hovCellColor_ = QColor::fromRgb(255, 255, 255, 64);
	rowNumColor_ = QColor::fromRgb(255, 200, 180);
	headerTextColor_ = QColor::fromRgb(240, 240, 200);
	headerRowColor_ = QColor::fromRgb(60, 60, 60);
	borderColor_ = QColor::fromRgb(120, 120, 120);


	initDisplay();

	setAttribute(Qt::WA_Hover);
}

void OrderListPanel::setCore(std::shared_ptr<BambooTracker> core)
{
	bt_ = core;
	curSongNum_ = bt_->getCurrentSongNumber();
	curPos_ = { bt_->getCurrentTrackAttribute().number, bt_->getCurrentOrderNumber() };
	modStyle_ = bt_->getModuleStyle();
	columnsWidthFromLeftToEnd_ = calculateColumnsWidthWithRowNum(0, modStyle_.trackAttribs.size() - 1);
}

void OrderListPanel::setCommandStack(std::weak_ptr<QUndoStack> stack)
{
	comStack_ = stack;
}

void OrderListPanel::initDisplay()
{
	pixmap_ = std::make_unique<QPixmap>(geometry().size());
}

void OrderListPanel::drawList(const QRect &rect)
{
	int maxWidth = std::min(geometry().width(), columnsWidthFromLeftToEnd_);

	pixmap_->fill(Qt::black);
	drawRows(maxWidth);
	drawHeaders(maxWidth);
	drawBorders(maxWidth);
	if (!hasFocus()) drawShadow();

	QPainter painter(this);
	painter.drawPixmap(rect, *pixmap_.get());
}

void OrderListPanel::drawRows(int maxWidth)
{
	QPainter painter(pixmap_.get());
	painter.setFont(rowFont_);

	std::vector<std::vector<int>> lists_;
	int x, trackNum;

	/* Current row */
	// Fill row
	painter.fillRect(0, curRowY_, maxWidth, rowFontHeight_,
					 bt_->isJamMode() ? curRowColor_ : curRowColorEditable_);
	// Row number
	painter.setPen(rowNumColor_);
	painter.drawText(1, curRowBaselineY_, QString("%1").arg(curPos_.row, 2, 16, QChar('0')).toUpper());
	// Step data
	painter.setPen(curTextColor_);
	for (x = rowNumWidth_ + widthSpace_, trackNum = leftTrackNum_; x < maxWidth; ) {
		lists_.push_back(bt_->getOrderList(curSongNum_, trackNum));
		if (trackNum == curPos_.track)	// Paint current cell
			painter.fillRect(x - widthSpace_, curRowY_, trackWidth_, rowFontHeight_, curCellColor_);
		if (((hovPos_.row == curPos_.row || hovPos_.row == -2) && hovPos_.track == trackNum)
				|| (hovPos_.track == -2 && hovPos_.row == curPos_.row))	// Paint hover
			painter.fillRect(x - widthSpace_, curRowY_, trackWidth_, rowFontHeight_, hovCellColor_);
		if ((selLeftAbovePos_.track >= 0 && selLeftAbovePos_.row >= 0)
				&& isSelectedCell(trackNum, curPos_.row))	// Paint selected
			painter.fillRect(x - widthSpace_, curRowY_, trackWidth_, rowFontHeight_, selCellColor_);
		painter.drawText(
					x,
					curRowBaselineY_,
					QString(" %1")
					.arg(lists_[trackNum - leftTrackNum_].at(curPos_.row), 2, 16, QChar('0')).toUpper()
				);

		x += trackWidth_;
		++trackNum;
	}

	int rowNum;
	int rowY, baseY, endY;

	/* Previous rows */
	endY = std::max(headerHeight_ - rowFontHeight_, curRowY_ - rowFontHeight_ * curPos_.row);
	for (rowY = curRowY_ - rowFontHeight_, baseY = curRowBaselineY_ - rowFontHeight_, rowNum = curPos_.row - 1;
		 rowY >= endY;
		 rowY -= rowFontHeight_, baseY -= rowFontHeight_, --rowNum) {
		// Fill row
		painter.fillRect(0, rowY, maxWidth, rowFontHeight_, defRowColor_);
		// Row number
		painter.setPen(rowNumColor_);
		painter.drawText(1, baseY, QString("%1").arg(rowNum, 2, 16, QChar('0')).toUpper());
		painter.setPen(defTextColor_);
		for (x = rowNumWidth_ + widthSpace_, trackNum = leftTrackNum_; x < maxWidth; ) {
			if (((hovPos_.row == rowNum || hovPos_.row == -2) && hovPos_.track == trackNum)
					|| (hovPos_.track == -2 && hovPos_.row == rowNum))	// Paint hover
				painter.fillRect(x - widthSpace_, rowY, trackWidth_, rowFontHeight_, hovCellColor_);
			if ((selLeftAbovePos_.track >= 0 && selLeftAbovePos_.row >= 0)
					&& isSelectedCell(trackNum, rowNum))	// Paint selected
				painter.fillRect(x - widthSpace_, rowY, trackWidth_, rowFontHeight_, selCellColor_);
			painter.drawText(
						x,
						baseY,
						QString(" %1").arg(lists_[trackNum - leftTrackNum_].at(rowNum), 2, 16, QChar('0')).toUpper()
					);

			x += trackWidth_;
			++trackNum;
		}
	}

	/* Next rows */
	endY = std::min(geometry().height(),
					curRowY_ + rowFontHeight_ * (static_cast<int>(lists_[0].size()) - curPos_.row - 1));
	for (rowY = curRowY_ + rowFontHeight_, baseY = curRowBaselineY_ + rowFontHeight_, rowNum = curPos_.row + 1;
		 rowY <= endY;
		 rowY += rowFontHeight_, baseY += rowFontHeight_, ++rowNum) {
		// Fill row
		painter.fillRect(0, rowY, maxWidth, rowFontHeight_, defRowColor_);
		// Row number
		painter.setPen(rowNumColor_);
		painter.drawText(1, baseY, QString("%1").arg(rowNum, 2, 16, QChar('0')).toUpper());
		painter.setPen(defTextColor_);
		for (x = rowNumWidth_ + widthSpace_, trackNum = leftTrackNum_; x < maxWidth; ) {
			if (((hovPos_.row == rowNum || hovPos_.row == -2) && hovPos_.track == trackNum)
					|| (hovPos_.track == -2 && hovPos_.row == rowNum))	// Paint hover
				painter.fillRect(x - widthSpace_, rowY, trackWidth_, rowFontHeight_, hovCellColor_);
			if ((selLeftAbovePos_.track >= 0 && selLeftAbovePos_.row >= 0)
					&& isSelectedCell(trackNum, rowNum))	// Paint selected
				painter.fillRect(x - widthSpace_, rowY, trackWidth_, rowFontHeight_, selCellColor_);
			painter.drawText(
						x,
						baseY,
						QString(" %1").arg(lists_[trackNum - leftTrackNum_].at(rowNum), 2, 16, QChar('0')).toUpper()
					);

			x += trackWidth_;
			++trackNum;
		}
	}
}

void OrderListPanel::drawHeaders(int maxWidth)
{
	QPainter painter(pixmap_.get());
	painter.setFont(headerFont_);

	painter.fillRect(0, 0, geometry().width(), headerHeight_, headerRowColor_);
	painter.setPen(headerTextColor_);
	int x, trackNum;
	for (x = rowNumWidth_ + widthSpace_, trackNum = leftTrackNum_; x < maxWidth; ) {
		QString str;
		switch (modStyle_.trackAttribs[trackNum].source) {
		case SoundSource::FM:	str = " FM";	break;
		case SoundSource::SSG:	str = " SSG";	break;
		}
		painter.drawText(x,
						 rowFontLeading_ + rowFontAscend_,
						 str + QString::number(modStyle_.trackAttribs[trackNum].channelInSource + 1));

		x += trackWidth_;
		++trackNum;
	}
}

void OrderListPanel::drawBorders(int maxWidth)
{
	QPainter painter(pixmap_.get());

	painter.drawLine(0, headerHeight_, geometry().width(), headerHeight_);
	painter.drawLine(rowNumWidth_, 0, rowNumWidth_, geometry().height());
	int x, trackNum;
	for (x = rowNumWidth_ + trackWidth_, trackNum = leftTrackNum_; x <= maxWidth; ) {
		painter.drawLine(x, 0, x, geometry().height());
		x += trackWidth_;
		++trackNum;
	}
}

void OrderListPanel::drawShadow()
{
	QPainter painter(pixmap_.get());
	painter.fillRect(0, 0, geometry().width(), geometry().height(), QColor::fromRgb(0, 0, 0, 47));
}

int OrderListPanel::calculateColumnsWidthWithRowNum(int begin, int end) const
{
	int width = rowNumWidth_;
	for (int i = begin; i <= end; ++i) {
		width +=  trackWidth_;
	}
	return width;
}

void OrderListPanel::moveCursorToRight(int n)
{
	if (n > 0) {
		curPos_.track = std::min(curPos_.track + n, static_cast<int>(modStyle_.trackAttribs.size()) - 1);
		while (calculateColumnsWidthWithRowNum(leftTrackNum_, curPos_.track) > geometry().width())
			++leftTrackNum_;
	}
	else {
		curPos_.track = std::max(curPos_.track + n, 0);
		if (curPos_.track < leftTrackNum_) leftTrackNum_ = curPos_.track;
	}
	columnsWidthFromLeftToEnd_
			= calculateColumnsWidthWithRowNum(leftTrackNum_, modStyle_.trackAttribs.size() - 1);

	if (!isIgnoreToSlider_) emit currentTrackChangedForSlider(curPos_.track);	// Send to slider

	if (!isIgnoreToPattern_) emit currentTrackChanged(curPos_.track);	// Send to pattern editor

	update();
}

void OrderListPanel::moveCursorToDown(int n)
{
	int tmp = curPos_.row + n;

	if (n > 0) {
		int endRow = bt_->getOrderList(curSongNum_, 0).size();
		if (tmp < endRow) {
			curPos_.row = tmp;
		}
		else {
			curPos_.row = endRow - 1;
		}
	}
	else {
		if (tmp < 0) {
			curPos_.row = 0;
		}
		else {
			curPos_.row = tmp;
		}
	}

	if (!isIgnoreToSlider_) emit currentOrderChangedForSlider(curPos_.row);	// Send to slider

	if (!isIgnoreToPattern_) emit currentOrderChanged(curPos_.row);	// Send to pattern editor

	update();
}

void OrderListPanel::changeEditable()
{
	update();
}

bool OrderListPanel::enterOrder(int key)
{
	switch (key) {
	case Qt::Key_0:	setCellOrderNum(0x0);	return true;
	case Qt::Key_1:	setCellOrderNum(0x1);	return true;
	case Qt::Key_2:	setCellOrderNum(0x2);	return true;
	case Qt::Key_3:	setCellOrderNum(0x3);	return true;
	case Qt::Key_4:	setCellOrderNum(0x4);	return true;
	case Qt::Key_5:	setCellOrderNum(0x5);	return true;
	case Qt::Key_6:	setCellOrderNum(0x6);	return true;
	case Qt::Key_7:	setCellOrderNum(0x7);	return true;
	case Qt::Key_8:	setCellOrderNum(0x8);	return true;
	case Qt::Key_9:	setCellOrderNum(0x9);	return true;
	case Qt::Key_A:	setCellOrderNum(0xa);	return true;
	case Qt::Key_B:	setCellOrderNum(0xb);	return true;
	case Qt::Key_C:	setCellOrderNum(0xc);	return true;
	case Qt::Key_D:	setCellOrderNum(0xd);	return true;
	case Qt::Key_E:	setCellOrderNum(0xe);	return true;
	case Qt::Key_F:	setCellOrderNum(0xf);	return true;
	default:	return false;
	}
}

void OrderListPanel::setCellOrderNum(int n)
{
	entryCnt_ = (entryCnt_ == 1 && curPos_ == editPos_) ? 0 : 1;
	editPos_ = curPos_;
	bt_->setOrderPattern(curSongNum_, editPos_.track, editPos_.row, n);
	comStack_.lock()->push(new SetPatternToOrderQtCommand(this, editPos_));

	if (!entryCnt_) moveCursorToDown(1);
}

void OrderListPanel::insertOrderBelow()
{
	bt_->insertOrderBelow(curSongNum_, curPos_.row);
	comStack_.lock()->push(new InsertOrderBelowQtCommand(this));
	moveCursorToDown(1);
}

void OrderListPanel::deleteOrder()
{
	if (bt_->getOrderList(curSongNum_, 0).size() > 1) {
		// KEEP CODE ORDER //
		bt_->deleteOrder(curSongNum_, curPos_.row);
		moveCursorToDown(-1);
		comStack_.lock()->push(new DeleteOrderQtCommand(this));
		//----------//
	}
}

void OrderListPanel::setSelectedRectangle(const OrderPosition& start, const OrderPosition& end)
{
	if (start.track > end.track) {
		if (start.row > end.row) {
			selLeftAbovePos_ = end;
			selRightBelowPos_ = start;
		}
		else {
			selLeftAbovePos_ = { end.track, start.row };
			selRightBelowPos_ = { start.track, end.row };
		}
	}
	else {
		if (start.row > end.row) {
			selLeftAbovePos_ = { start.track, end.row };
			selRightBelowPos_ = { end.track, start.row };
		}
		else {
			selLeftAbovePos_ = start;
			selRightBelowPos_ = end;
		}
	}
}

bool OrderListPanel::isSelectedCell(int track, int row)
{
	OrderPosition pos{ track, row };
	return (selLeftAbovePos_.track <= pos.track && selRightBelowPos_.track >= pos.track
			&& selLeftAbovePos_.row <= pos.row && selRightBelowPos_.row >= pos.row);
}

/********** Slots **********/
void OrderListPanel::setCurrentTrackForSlider(int num)
{
	Ui::EventGuard eg(isIgnoreToSlider_);

	if (int dif = num - curPos_.track) moveCursorToRight(dif);
}

void OrderListPanel::setCurrentOrderForSlider(int num) {
	Ui::EventGuard eg(isIgnoreToSlider_);

	if (int dif = num - curPos_.row) moveCursorToDown(dif);
}

void OrderListPanel::setCurrentTrack(int num)
{
	Ui::EventGuard eg(isIgnoreToPattern_);

	if (int dif = num - curPos_.track) moveCursorToRight(dif);
}

void OrderListPanel::setCurrentOrder(int num) {
	Ui::EventGuard eg(isIgnoreToPattern_);

	if (int dif = num - curPos_.row) moveCursorToDown(dif);
}

void OrderListPanel::onOrderEdited()
{
	emit orderEdited();
}

/********** Events **********/
bool OrderListPanel::event(QEvent *event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
		return keyPressed(dynamic_cast<QKeyEvent*>(event));
	case QEvent::KeyRelease:
		return keyReleased(dynamic_cast<QKeyEvent*>(event));
	case QEvent::HoverMove:
		return mouseHoverd(dynamic_cast<QHoverEvent*>(event));
	default:
		return QWidget::event(event);
	}
}

bool OrderListPanel::keyPressed(QKeyEvent *event)
{	
	/* General Keys (with Ctrl) */
	if (event->modifiers().testFlag(Qt::ControlModifier)) return false;

	/* General Keys */
	switch (event->key()) {
	case Qt::Key_Shift:
		shiftPressedPos_ = curPos_;
		return true;
	case Qt::Key_Left:
		moveCursorToRight(-1);
		if (event->modifiers().testFlag(Qt::ShiftModifier))
			setSelectedRectangle(shiftPressedPos_, curPos_);
		return true;
	case Qt::Key_Right:
		moveCursorToRight(1);
		if (event->modifiers().testFlag(Qt::ShiftModifier))
			setSelectedRectangle(shiftPressedPos_, curPos_);
		return true;
	case Qt::Key_Up:
		if (bt_->isPlaySong()) {
			return false;
		}
		else {
			moveCursorToDown(-1);
			if (event->modifiers().testFlag(Qt::ShiftModifier)
					&& shiftPressedPos_.row == curPos_.row) {
				setSelectedRectangle(shiftPressedPos_, curPos_);
				return true;
			}
			return true;
		}
	case Qt::Key_Down:
		if (bt_->isPlaySong()) {
			return false;
		}
		else {
			moveCursorToDown(1);
			if (event->modifiers().testFlag(Qt::ShiftModifier)
					&& shiftPressedPos_.row == curPos_.row) {
				setSelectedRectangle(shiftPressedPos_, curPos_);
				return true;
			}
			return true;
		}
	case Qt::Key_Insert:
		if (bt_->isJamMode()) {
			return false;
		}
		else {
			insertOrderBelow();
			return true;
		}
	case Qt::Key_Delete:
		if (bt_->isJamMode()) {
			return false;
		}
		else {
			deleteOrder();
			return true;
		}
	default:
		if (!bt_->isJamMode()) {
			return enterOrder(event->key());
		}
		return false;
	}
}

bool OrderListPanel::keyReleased(QKeyEvent* event)
{
	switch (event->key()) {
	case Qt::Key_Shift:
		shiftPressedPos_ = { -1, -1 };
		return true;
	default:
		return false;
	}
}

void OrderListPanel::paintEvent(QPaintEvent *event)
{
	if (bt_ != nullptr) drawList(event->rect());
}

void OrderListPanel::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	// Recalculate center row position
	curRowBaselineY_ = (geometry().height() - headerHeight_) / 2 + headerHeight_;
	curRowY_ = curRowBaselineY_ - (rowFontAscend_ + rowFontLeading_ / 2);
	initDisplay();
}

void OrderListPanel::mousePressEvent(QMouseEvent *event)
{
	Q_UNUSED(event)

	setFocus();

	mousePressPos_ = hovPos_;
	mouseReleasePos_ = { -1, -1 };

	if (event->button() == Qt::LeftButton) {
		selLeftAbovePos_ = { -1, -1 };
		selRightBelowPos_ = { -1, -1 };
	}
}

void OrderListPanel::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton) {
		if (mousePressPos_.track < 0 || mousePressPos_.row < 0) return;	// Start point is out of range

		if (hovPos_.track >= 0) {
			setSelectedRectangle(mousePressPos_, hovPos_);
			update();
		}

		if (event->x() < rowNumWidth_ && leftTrackNum_ > 0) {
			moveCursorToRight(-1);
		}
		else if (event->x() > geometry().width() - rowNumWidth_ && hovPos_.track != -1) {
			moveCursorToRight(1);
		}
		if (event->pos().y() < headerHeight_ + rowFontHeight_) {
			moveCursorToDown(-1);
		}
		else if (event->pos().y() > geometry().height() - rowFontHeight_) {
			moveCursorToDown(1);
		}
	}
}

void OrderListPanel::mouseReleaseEvent(QMouseEvent* event)
{
	mouseReleasePos_ = hovPos_;

	if (event->button() == Qt::LeftButton) {
		if (mousePressPos_ == mouseReleasePos_) {	// Jump cell
			if (hovPos_.row >= 0 && hovPos_.track >= 0) {
				int horDif = hovPos_.track - curPos_.track;
				int verDif = hovPos_.row - curPos_.row;
				moveCursorToRight(horDif);
				moveCursorToDown(verDif);
				update();
			}
			else if (hovPos_.row == -2 && hovPos_.track >= 0) {	// Header
				int horDif = hovPos_.track - curPos_.track;
				moveCursorToRight(horDif);
				update();
			}
			else if (hovPos_.track == -2 && hovPos_.row >= 0) {	// Row number
				int verDif = hovPos_.row - curPos_.row;
				moveCursorToDown(verDif);
				update();
			}
		}
	}

	mousePressPos_ = { -1, -1 };
	mouseReleasePos_ = { -1, -1 };
}

bool OrderListPanel::mouseHoverd(QHoverEvent *event)
{
	QPoint pos = event->pos();

	OrderPosition oldPos = hovPos_;

	// Detect row
	if (pos.y() <= headerHeight_) {
		 hovPos_.row = -2;	// Header
	}
	else {
		if (pos.y() < curRowY_) {
			int tmp = curPos_.row + (pos.y() - curRowY_) / rowFontHeight_ - 1;
			hovPos_.row = (tmp < 0) ? -1 : tmp;
		}
		else {
			hovPos_.row = curPos_.row + (pos.y() - curRowY_) / rowFontHeight_;
			if (hovPos_.row >= bt_->getOrderList(curSongNum_, 0).size()) hovPos_.row = -1;
		}
	}

	// Detect track
	if (pos.x() <= rowNumWidth_) {
		hovPos_.track = -2;	// Row number
	}
	else {
		int tmpWidth = rowNumWidth_;
		for (int i = leftTrackNum_; ; ) {
			tmpWidth += trackWidth_;
			if (pos.x() <= tmpWidth) {
				hovPos_.track = i;
				break;
			}
			++i;

			if (i == modStyle_.trackAttribs.size()) {
				hovPos_.track = -1;
				break;
			}
		}
	}

	if (hovPos_ != oldPos) update();

	return true;
}

void OrderListPanel::wheelEvent(QWheelEvent *event)
{
	int degree = event->angleDelta().y() / 8;
	moveCursorToDown(-degree / 15);
}

void OrderListPanel::leaveEvent(QEvent* event)
{
	Q_UNUSED(event)
	// Clear mouse hover selection
	hovPos_ = { -1, -1 };
}
