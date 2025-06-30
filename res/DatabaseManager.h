#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime> // 新增，因为要处理 QDateTime
#include <QMap>      // 新增，用于返回 QMap<QDate, int>
#include <QDate>     // 新增，因为要处理 QDate
#include "DailyTask.h" // 确保你的 DailyTask.h 存在

class DatabaseManager
{
public:
    bool deleteAllStudySessions(); // 【新增】删除所有自习记录
    QList<QDate> getAllDatesWithTasks() const;
    static DatabaseManager& instance();
    bool init();
    //增
    bool addDailyTask(const QDate &date, DailyTask &task);
    QList<DailyTask> getTasksForDate(const QDate &date);
    //删
    bool deleteTaskById(int id);
    //改
    bool updateTaskById(int id, const DailyTask &task);

    bool addStudySession(const QDateTime &start, const QDateTime &end, int durationSeconds);
    // 新增：获取每日自习时长
    QMap<QDate, int> getDailyStudyDurations();

private:
    DatabaseManager(); // 单例
    QSqlDatabase db;
};

#endif // DATABASEMANAGER_H
