#ifndef DAILYTASK_H
#define DAILYTASK_H

#include <QString>
#include <QTime>
#include <QDate> // 包含QDate头文件

class DailyTask {
public:
    DailyTask(const QString &title = "",
              const QTime &start = QTime(),
              const QTime &end = QTime(),
              const QString &note = "",
              int id = -1);

    QString getTitle() const;
    QTime getStartTime() const;
    QTime getEndTime() const;
    QString getNote() const;
    int getId() const;

    void setTitle(const QString &t) { title = t; }
    void setStartTime(const QTime &t) { startTime = t; }
    void setEndTime(const QTime &t) { endTime = t; }
    void setNote(const QString &n) { note = n; }
    void setId(const int value) { id = value; }

private:
    int id = -1;
    QString title;
    QTime startTime;
    QTime endTime;
    QString note;
};



#endif // DAILYTASK_H
