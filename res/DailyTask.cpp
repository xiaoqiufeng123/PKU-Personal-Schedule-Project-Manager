#include "DailyTask.h"

DailyTask::DailyTask(const QString &title_, const QTime &start_, const QTime &end_, const QString &note_, int id_)
    : title(title_),  startTime(start_), endTime(end_), note(note_), id(id_) {}

QString DailyTask::getTitle() const { return title; }
QTime DailyTask::getStartTime() const { return startTime; }
QTime DailyTask::getEndTime() const { return endTime; }
QString DailyTask::getNote() const { return note; }
int DailyTask::getId() const { return id; }
