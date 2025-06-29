#include "DatabaseManager.h"

DatabaseManager::DatabaseManager() {}


// 【新增】删除所有自习记录的实现
bool DatabaseManager::deleteAllStudySessions()
{
    if (!db.isOpen()) {
        qWarning() << "数据库未打开，无法删除记录！";
        return false;
    }
    QSqlQuery query(db);
    // 使用 DELETE FROM 语句清空表，这比 DROP TABLE 更安全
    if (!query.exec("DELETE FROM study_sessions")) {
        qWarning() << "清空 study_sessions 表失败: " << query.lastError().text();
        return false;
    }
    // VACUUM 命令可以收缩数据库文件，释放已删除数据占用的空间（可选）
    query.exec("VACUUM");
    return true;
}
QList<QDate> DatabaseManager::getAllDatesWithTasks() const
{
    QList<QDate> dates;
    if (db.isOpen()) {
        // 【关键修正】将查询的表名从 daily_tasks 改为 tasks
        QSqlQuery query("SELECT DISTINCT date FROM tasks");
        while (query.next()) {
            dates.append(query.value(0).toDate());
        }
    }
    return dates;
}
DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::init() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("tasks.db");
    if (!db.open()) {
        qDebug() << "无法打开数据库：" << db.lastError().text();
        return false;
    }

    QSqlQuery query;
    QString createTable = R"(
        CREATE TABLE IF NOT EXISTS tasks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            date TEXT,
            title TEXT,
            start_time TEXT,
            end_time TEXT,
            note TEXT
        )
    )";
    QString createStudyTable = R"(
    CREATE TABLE IF NOT EXISTS study_sessions (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        start_time TEXT,
        end_time TEXT,
        duration_seconds INTEGER
    )
    )";
    // 执行创建 study_sessions 表
    if (!query.exec(createStudyTable)) {
        qDebug() << "创建 study_sessions 表失败：" << query.lastError().text();
        return false;
    }

    // 执行创建 tasks 表
    if (!query.exec(createTable)) {
        qDebug() << "创建 tasks 表失败：" << query.lastError().text();
        return false;
    }

    return true;
}


bool DatabaseManager::addDailyTask(const QDate &date, DailyTask &task) {
    QSqlQuery query;
    query.prepare("INSERT INTO tasks (date, title, start_time, end_time, note) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(date.toString(Qt::ISODate));
    query.addBindValue(task.getTitle());
    query.addBindValue(task.getStartTime().toString("HH:mm"));
    query.addBindValue(task.getEndTime().toString("HH:mm"));
    query.addBindValue(task.getNote());
    bool success = query.exec();
    if (success) {
        task.setId(query.lastInsertId().toInt());  // ✅ 设置返回的 ID
    } else {
        qDebug() << "添加任务失败：" << query.lastError().text();
    }
    return success;
}

QList<DailyTask> DatabaseManager::getTasksForDate(const QDate &date) {
    QList<DailyTask> tasks;
    QSqlQuery query;
    query.prepare("SELECT title, start_time, end_time, note, id FROM tasks WHERE date = ?");
    query.addBindValue(date.toString(Qt::ISODate));
    if (query.exec()) {
        while (query.next()) {
            QString title = query.value(0).toString();
            QTime start = QTime::fromString(query.value(1).toString(), "HH:mm");
            QTime end = QTime::fromString(query.value(2).toString(), "HH:mm");
            QString note = query.value(3).toString();
            int id = query.value(4).toInt(); // 确保 SELECT 语句中有 id

            DailyTask task(title, start, end, note, id);
            tasks.append(task);
        }
    }
    return tasks;
}


bool DatabaseManager::deleteTaskById(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM tasks WHERE id = ?");
    query.addBindValue(id); // 绑定id
    if (!query.exec()) {
        qDebug() << "删除任务失败：" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::updateTaskById(int id, const DailyTask &task) {
    QSqlQuery query;
    query.prepare(R"(
        UPDATE tasks
        SET title = ?, start_time = ?, end_time = ?, note = ?
        WHERE id = ?
    )");
    query.addBindValue(task.getTitle());
    query.addBindValue(task.getStartTime().toString("HH:mm"));
    query.addBindValue(task.getEndTime().toString("HH:mm"));
    query.addBindValue(task.getNote());
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug() << "更新任务失败：" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::addStudySession(const QDateTime &start, const QDateTime &end, int durationSeconds)
{
    QSqlQuery query;
    query.prepare("INSERT INTO study_sessions (start_time, end_time, duration_seconds) VALUES (?, ?, ?)");
    query.addBindValue(start.toString(Qt::ISODate));
    query.addBindValue(end.toString(Qt::ISODate));
    query.addBindValue(durationSeconds);

    if (!query.exec()) {
        qDebug() << "保存自习记录失败：" << query.lastError().text();
        return false;
    }
    return true;
}

// 新增：获取每日自习时长
QMap<QDate, int> DatabaseManager::getDailyStudyDurations()
{
    QMap<QDate, int> dailyDurations;
    if (!db.isOpen()) {
        qWarning() << "Database is not open for getDailyStudyDurations!";
        return dailyDurations;
    }

    QSqlQuery query(db);
    // 从 study_sessions 表中查询 start_time 和 duration_seconds
    if (query.exec("SELECT start_time, duration_seconds FROM study_sessions ORDER BY start_time ASC")) {
        while (query.next()) {
            // start_time 是 TEXT 类型，需要转换为 QDateTime
            QDateTime startTime = QDateTime::fromString(query.value(0).toString(), Qt::ISODate);
            int duration = query.value(1).toInt();
            QDate date = startTime.date(); // 获取日期部分
            dailyDurations[date] += duration; // 累加每天的总时长
        }
    } else {
        qWarning() << "Error getting daily study durations:" << query.lastError().text();
    }
    return dailyDurations;
}
