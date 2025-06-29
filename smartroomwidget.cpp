#include "smartroomwidget.h"
#include <QComboBox>
#include <QListWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QProcess>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QRegularExpression>

SmartRoomWidget::SmartRoomWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("空闲教室查询和推荐");
    resize(800, 600);

    // 初始化顶部控件：楼宇选择 + 时间选择 + 查询按钮
    buildingBox = new QComboBox;
    buildingBox->addItems({
        "一教", "二教", "三教", "四教", "理教",
        "文史", "哲学", "地学楼", "国关", "政管"
    });

    timeBox = new QComboBox;
    timeBox->addItems({ "今天", "明天", "后天" });

    searchButton = new QPushButton("查询");
    connect(searchButton, &QPushButton::clicked,
            this, &SmartRoomWidget::onSearchClicked);

    // 创建顶部布局
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(new QLabel("楼宇："));
    topLayout->addWidget(buildingBox);
    topLayout->addSpacing(20);
    topLayout->addWidget(new QLabel("时间："));
    topLayout->addWidget(timeBox);
    topLayout->addSpacing(20);
    topLayout->addWidget(searchButton);
    topLayout->addStretch();

    // 创建节次筛选列表
    sectionList = new QListWidget;
    sectionList->setFixedHeight(240);
    for (int i = 1; i <= 12; ++i) {
        auto *item = new QListWidgetItem(QString("第%1节课").arg(i), sectionList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }

    QHBoxLayout *secLayout = new QHBoxLayout;
    secLayout->addWidget(new QLabel("筛选节次："));
    secLayout->addWidget(sectionList);

    // 创建结果表格和状态标签
    resultTable = new QTableWidget;
    resultTable->setColumnCount(2);
    resultTable->setHorizontalHeaderLabels({ "节次", "教室" });
    resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    statusLabel = new QLabel("状态：等待查询");

    // 设置主布局
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(secLayout);
    mainLayout->addWidget(resultTable);
    mainLayout->addWidget(statusLabel);
    setLayout(mainLayout);
}

void SmartRoomWidget::onSearchClicked()
{
    QString building = buildingBox->currentText();
    QString time = timeBox->currentText();

    // 准备Python脚本路径
    QString script = QCoreApplication::applicationDirPath() + "/query_free_rooms.py";
    QStringList arguments = { script, building, time };

    // 启动Python进程
    QProcess process;
    qDebug() << "执行命令: python" << script << building << time;
    process.start("python", arguments);

    // 等待进程完成（最多10秒）
    if (!process.waitForFinished(10000)) {
        statusLabel->setText("状态：脚本执行超时");
        qDebug() << "stdout:" << process.readAllStandardOutput();
        qDebug() << "stderr:" << process.readAllStandardError();
        qDebug() << "exitCode" << process.exitCode();
        return;
    }

    // 获取并处理脚本输出
    QString output = process.readAllStandardOutput().trimmed();
    qDebug() << "【Python输出】" << output;
    parseJsonAndDisplay(output);
}

void SmartRoomWidget::parseJsonAndDisplay(const QString &jsonText)
{
    // 清除旧数据
    resultTable->clearContents();
    resultTable->setRowCount(0);

    // 处理空响应
    QString payload = jsonText.trimmed();
    if (payload.isEmpty()) {
        qDebug() << "收到空JSON响应，使用默认空对象";
        payload = "{}";
    }

    // 解析JSON
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8(), &err);

    // 检查JSON解析错误
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        statusLabel->setText("状态：JSON解析错误");
        qDebug() << "解析错误:" << err.errorString() << "\n原始数据:" << payload;
        return;
    }

    QJsonObject root = doc.object();

    // 获取当前选择的楼宇
    QString building = buildingBox->currentText();
    QString actualBuilding = building;

    // 如果JSON中没有对应的楼宇键，使用第一个可用的键
    if (!root.contains(building)) {
        QStringList keys = root.keys();
        if (keys.isEmpty()) {
            statusLabel->setText("状态：无可用数据");
            return;
        }
        actualBuilding = keys.first();
        qDebug() << "未找到楼宇键" << building
                 << "，使用" << actualBuilding << "代替";
    }

    QJsonObject buildingObj = root.value(actualBuilding).toObject();

    // 获取用户选择的节次
    QStringList selectedSections;
    for (int i = 0; i < sectionList->count(); ++i) {
        QListWidgetItem *item = sectionList->item(i);
        if (item->checkState() == Qt::Checked) {
            QString section = item->text();
            // 提取节次数字（如"第3节课" -> "3"）
            section.remove(QRegularExpression("[^0-9]"));
            selectedSections << section;
        }
    }

    // 填充结果表格
    int recordCount = 0;
    for (const QString &dateKey : buildingObj.keys()) {
        QJsonObject sections = buildingObj.value(dateKey).toObject();

        // 跳过"default"日期键
        QString displayDate = (dateKey == "default" ? QString() : dateKey);

        for (const QString &sectionKey : sections.keys()) {
            // 提取节次数字（如"c3" -> "3"）
            QString sectionNum = sectionKey;
            sectionNum.remove(QRegularExpression("[^0-9]"));

            // 如果用户选择了特定节次，跳过未选中的
            if (!selectedSections.isEmpty() && !selectedSections.contains(sectionNum)) {
                continue;
            }

            // 获取该节次的空闲教室列表
            QJsonArray rooms = sections.value(sectionKey).toArray();
            for (const QJsonValue &roomValue : rooms) {
                QString room = roomValue.toString();
                if (room.isEmpty()) continue;

                // 添加行到结果表格
                int row = resultTable->rowCount();
                resultTable->insertRow(row);
                resultTable->setItem(row, 0, new QTableWidgetItem(sectionNum));
                resultTable->setItem(row, 1, new QTableWidgetItem(room));
                recordCount++;
            }
        }
    }

    // 更新状态标签
    if (recordCount == 0) {
        statusLabel->setText("状态：未找到空闲教室");
    } else {
        statusLabel->setText(QString("状态：共找到 %1 条记录").arg(recordCount));
    }
}
