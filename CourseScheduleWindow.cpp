#include "CourseScheduleWindow.h"
#include <QVBoxLayout>
#include <QTableWidget>
#include <QLabel>
#include <QHeaderView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTableWidgetItem>
#include <QSettings>
#include <QDebug>

const int NUM_DAYS = 7;
const int NUM_PERIODS = 12;

CourseScheduleWindow::CourseScheduleWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    m_scraperProcess = new QProcess(this);
    connect(m_scraperProcess, &QProcess::finished,
            this, &CourseScheduleWindow::onScraperFinished);

    // --- 新增：初始化空闲教室查询进程并连接信号 ---
    m_freeRoomQueryProcess = new QProcess(this);
    connect(m_freeRoomQueryProcess, &QProcess::finished,
            this, &CourseScheduleWindow::onFreeRoomQueryFinished);

    // --- 新增：连接表格点击事件到新的槽函数 ---
    connect(m_scheduleTable, &QTableWidget::cellClicked,
            this, &CourseScheduleWindow::onTableCellClicked);

    setAcceptDrops(true);

    loadSchedule();
}

CourseScheduleWindow::~CourseScheduleWindow()
{
    // 确保所有子进程在窗口关闭时都被正确终止
    if (m_scraperProcess && m_scraperProcess->state() == QProcess::Running) {
        m_scraperProcess->kill();
    }
    if (m_freeRoomQueryProcess && m_freeRoomQueryProcess->state() == QProcess::Running) {
        m_freeRoomQueryProcess->kill();
    }
}

void CourseScheduleWindow::setupUi()
{
    // 更新窗口标题和提示信息，引导新交互
    setWindowTitle("我的课表 (点击空白处查询空闲教室)");
    resize(800, 600);

    m_infoLabel = new QLabel(
        "请拖拽课表HTML文件到此，或直接点击课表空白处查询当日空闲教室",
        this
        );
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setMinimumHeight(80);
    m_infoLabel->setStyleSheet(
        "QLabel { border: 2px dashed #aaa; border-radius: 5px; font-size: 16px; color: #555; }"
        );

    m_scheduleTable = new QTableWidget(NUM_PERIODS, NUM_DAYS, this);
    m_scheduleTable->setHorizontalHeaderLabels(
        {"一", "二", "三", "四", "五", "六", "日"}
        );
    QStringList periodLabels;
    for (int i = 1; i <= NUM_PERIODS; ++i)
        periodLabels << QString::number(i);
    m_scheduleTable->setVerticalHeaderLabels(periodLabels);
    m_scheduleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_scheduleTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_infoLabel);
    mainLayout->addWidget(m_scheduleTable);
    setLayout(mainLayout);
}

void CourseScheduleWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void CourseScheduleWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        if (!urlList.isEmpty()) {
            QString filePath = urlList.first().toLocalFile();
            QFileInfo fileInfo(filePath);
            if (fileInfo.suffix().toLower() == "html" ||
                fileInfo.suffix().toLower() == "txt") {
                m_infoLabel->setText(
                    QString("正在解析文件: %1").arg(fileInfo.fileName())
                    );
                startScraperWithFile(filePath);
            } else {
                QMessageBox::warning(
                    this,
                    "文件类型错误",
                    "请拖拽 .html 或 .txt 文件。"
                    );
            }
        }
    }
}

void CourseScheduleWindow::startScraperWithFile(const QString &filePath)
{
    if (m_scraperProcess->state() == QProcess::Running)
        return;

    m_scheduleTable->clearContents();
    QString pythonExecutable = "python";
    QStringList args;
    args << "-B" << "scraper.py" << filePath;
    m_scraperProcess->start(pythonExecutable, args);
}

void CourseScheduleWindow::onScraperFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_infoLabel->setText("解析完成！请拖拽新的文件来更新课表。");
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        QMessageBox::critical(
            this,
            "错误",
            "Python脚本执行失败！\n请检查文件内容或Python环境。"
            );
        qDebug() << "Python script error output:"
                 << m_scraperProcess->readAllStandardError();
        return;
    }

    QByteArray jsonData = m_scraperProcess->readAllStandardOutput();

    if (!jsonData.isEmpty()) {
        saveSchedule(jsonData);
    }

    populateTable(jsonData);
}

void CourseScheduleWindow::populateTable(const QByteArray& jsonData)
{
    if (jsonData.isEmpty()) {
        QMessageBox::warning(
            this,
            "提示",
            "未能获取到课表数据，文件可能是空的或格式不正确。"
            );
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull() || !doc.isArray()) {
        qDebug() << "Failed to parse JSON or JSON is not an array. Data:"
                 << jsonData;
        QMessageBox::critical(
            this,
            "错误",
            "无法解析课表数据！文件内容可能不符合预期格式。"
            );
        return;
    }

    QJsonArray scheduleArray = doc.array();
    m_scheduleTable->clearContents();

    if (scheduleArray.isEmpty()) {
        QMessageBox::information(
            this,
            "提示",
            "成功解析文件，但未找到课程信息。"
            );
        return;
    }

    for (const QJsonValue& value : scheduleArray) {
        QJsonObject course = value.toObject();
        int day = course["day"].toInt() - 1;
        int startPeriod = course["start_period"].toInt() - 1;
        int periods = course["periods"].toInt();

        if (day < 0 || day >= NUM_DAYS ||
            startPeriod < 0 || startPeriod >= NUM_PERIODS)
            continue;

        auto* item = new QTableWidgetItem();
        QString displayText = QString("%1\n\n@%2\n%3")
                                  .arg(course["name"].toString())
                                  .arg(course["classroom"].toString())
                                  .arg(course["teacher"].toString());
        item->setText(displayText);
        item->setTextAlignment(Qt::AlignCenter);
        item->setBackground(QColor(course["color"].toString()));
        item->setToolTip(displayText);
        m_scheduleTable->setItem(startPeriod, day, item);

        if (periods > 1) {
            m_scheduleTable->setSpan(startPeriod, day, periods, 1);
        }
    }
}

void CourseScheduleWindow::saveSchedule(const QByteArray& jsonData)
{
    QSettings settings("MyCourseApp", "ScheduleData");
    settings.setValue("lastScheduleJson", jsonData);
    qDebug() << "课表数据已保存。";
}

void CourseScheduleWindow::loadSchedule()
{
    QSettings settings("MyCourseApp", "ScheduleData");
    QByteArray savedJson = settings.value("lastScheduleJson").toByteArray();
    if (!savedJson.isEmpty()) {
        qDebug() << "找到已保存的课表数据，正在加载...";
        m_infoLabel->setText("已加载上次保存的课表，可拖拽文件更新");
        populateTable(savedJson);
    } else {
        qDebug() << "未找到已保存的课表数据。";
    }
}

// --- 新增：实现单元格点击的槽函数 ---
void CourseScheduleWindow::onTableCellClicked(int row, int column)
{
    // 检查查询进程是否已在运行
    if (m_freeRoomQueryProcess->state() == QProcess::Running) {
        QMessageBox::information(
            this,
            "提示",
            "正在执行上一个查询，请稍候..."
            );
        return;
    }

    // 检查点击的是否是空白格子 (没有课程安排)
    if (m_scheduleTable->item(row, column) == nullptr) {
        // 弹出对话框，让用户选择教学楼
        QStringList buildings = {
            "一教", "二教", "三教", "四教", "理教",
            "文史", "哲学", "地学楼", "国关", "政管"
        };
        bool ok;
        QString building = QInputDialog::getItem(
            this,
            "查询空闲教室",
            QString("查询时间：周%1 第 %2 节课\n请选择教学楼：")
                .arg(m_scheduleTable->horizontalHeaderItem(column)->text())
                .arg(row + 1),
            buildings,
            0,
            false,
            &ok
            );

        if (ok && !building.isEmpty()) {
            m_infoLabel->setText(
                QString("正在查询 %1 的空闲教室...").arg(building)
                );

            // 保存查询上下文，以便在完成时使用
            m_lastQueriedPeriod = row + 1;
            m_lastQueriedBuilding = building;

            // 调用Python脚本，这里我们默认查询“今天”的空闲情况
            // 注意：请确保 query_free_room.py 位于可执行文件同目录下
            QString pythonExecutable = "python";
            QStringList args;
            args << "query_free_room.py" << building << "今天";
            m_freeRoomQueryProcess->start(pythonExecutable, args);
        }
    }
}

// --- 新增：实现处理查询结果的槽函数 ---
void CourseScheduleWindow::onFreeRoomQueryFinished()
{
    m_infoLabel->setText("查询完成！可继续点击空白处查询，或拖拽文件更新课表。");

    if (m_freeRoomQueryProcess->exitStatus() != QProcess::NormalExit ||
        m_freeRoomQueryProcess->exitCode() != 0) {
        QMessageBox::critical(
            this,
            "查询失败",
            "Python脚本执行出错。\n请检查Python环境或脚本文件是否存在。"
            );
        qDebug() << "Free room script error:"
                 << m_freeRoomQueryProcess->readAllStandardError();
        return;
    }

    QByteArray jsonData = m_freeRoomQueryProcess->readAllStandardOutput();
    if (jsonData.isEmpty()) {
        QMessageBox::warning(
            this,
            "无结果",
            "查询脚本没有返回任何数据。"
            );
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isObject()) {
        QMessageBox::critical(
            this,
            "解析失败",
            "无法解析返回的JSON数据。"
            );
        qDebug() << "Could not parse JSON:" << jsonData;
        return;
    }

    // 解析JSON，找到对应教学楼、对应节次的教室列表
    QJsonObject rootObj = doc.object();
    QJsonObject buildingObj = rootObj.value(m_lastQueriedBuilding).toObject();

    // 在返回的数据中，日期可能是"default"或其他，节次是 "c1", "c2" 等
    QString sectionKey = "c" + QString::number(m_lastQueriedPeriod);
    QStringList freeRooms;

    for (const QString& dateKey : buildingObj.keys()) {
        QJsonObject sectionsObj = buildingObj.value(dateKey).toObject();
        if (sectionsObj.contains(sectionKey)) {
            QJsonArray roomsArray = sectionsObj.value(sectionKey).toArray();
            for (const QJsonValue& val : roomsArray) {
                freeRooms.append(val.toString());
            }
        }
    }

    // 显示最终结果
    QString title = QString("周%1 第%2节 %3 空闲教室")
                        .arg(m_scheduleTable->horizontalHeaderItem(m_scheduleTable->currentColumn())->text())
                        .arg(m_lastQueriedPeriod)
                        .arg(m_lastQueriedBuilding);

    if (freeRooms.isEmpty()) {
        QMessageBox::information(
            this,
            title,
            "未找到该时段的空闲教室。"
            );
    } else {
        QMessageBox::information(
            this,
            title,
            "可用教室：\n" + freeRooms.join(", ")
            );
    }
}
