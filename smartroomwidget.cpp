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

    // 顶部：楼宇 + 时间 + 查询按钮
    buildingBox = new QComboBox;
    buildingBox->addItems({"一教",	"二教",	"三教",	"四教","理教",	"文史",	"哲学",	"地学楼","国关",	"政管"});

    timeBox = new QComboBox;
    timeBox->addItems({ "今天", "明天","后天" });

    searchButton = new QPushButton("查询");
    connect(searchButton, &QPushButton::clicked, this, &SmartRoomWidget::onSearchClicked);

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(new QLabel("楼宇："));
    topLayout->addWidget(buildingBox);
    topLayout->addSpacing(20);
    topLayout->addWidget(new QLabel("时间："));
    topLayout->addWidget(timeBox);
    topLayout->addSpacing(20);
    topLayout->addWidget(searchButton);
    topLayout->addStretch();

    // 中部：节次筛选
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

    // 下部：结果表格 + 状态栏
    resultTable = new QTableWidget;
    resultTable->setColumnCount(2);
    resultTable->setHorizontalHeaderLabels({ "节次", "教室" });
    resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    statusLabel = new QLabel("状态：等待查询");

    // 总布局
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
    QString time     = timeBox->currentText();

    // 调用 Python 脚本
    QProcess process;
    QString script = QCoreApplication::applicationDirPath() + "/query_free_rooms.py";
    process.start("python", QStringList() << script << building << time);
    qDebug() << "running:" << "python3" << script << building << time;
    if (!process.waitForFinished(10000)) {
        statusLabel->setText("状态：脚本超时");
        qDebug() << "stdout:" << process.readAllStandardOutput();
        qDebug() << "stderr:" << process.readAllStandardError();
        qDebug() << "exitCode" << process.exitCode();

        return;
    }
    QString output = process.readAllStandardOutput().trimmed();
    qDebug() << "【Python 输出】" << output;
    parseJsonAndDisplay(output);
}

void SmartRoomWidget::parseJsonAndDisplay(const QString &jsonText)
{
    // 1) 清空旧表格
    resultTable->clearContents();
    resultTable->setRowCount(0);

    // 2) 防止空串
    QString payload = jsonText.trimmed();
    if (payload.isEmpty()) {
        qDebug() << "Empty JSON payload, substituting {}";
        payload = "{}";
    }

    // 3) 解析 JSON
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        statusLabel->setText("状态：JSON 格式错误");
        qDebug() << "Parse error:" << err.errorString() << "\nRaw:" << payload;
        return;
    }
    QJsonObject root = doc.object();

    // 4) 确定实际的楼宇键
    QString building = buildingBox->currentText();
    QString actualBuilding = building;
    if (!root.contains(building)) {
        // 回退到 JSON 中的第一个键
        QStringList keys = root.keys();
        if (keys.isEmpty()) {
            statusLabel->setText("状态：无数据返回");
            return;
        }
        actualBuilding = keys.first();
        qDebug() << "Building key not found, using" << actualBuilding << "instead";
    }
    QJsonObject buildingObj = root.value(actualBuilding).toObject();

    // 5) 收集用户勾选的节次数字列表（"1","2",...）
    QStringList selected;
    for (int i = 0; i < sectionList->count(); ++i) {
        auto *it = sectionList->item(i);
        if (it->checkState() == Qt::Checked) {
            QString sec = it->text();            // e.g. "第3节课"
            sec.remove(QRegularExpression("[^0-9]")); // 保留数字 "3"
            selected << sec;
        }
    }

    // 6) 填表：日期→节次→教室
    int count = 0;
    for (const QString &dateKey : buildingObj.keys()) {
        QJsonObject secs = buildingObj.value(dateKey).toObject();
        // 如果是"default"，可以显示为空或自定义
        QString displayDate = (dateKey == "default" ? QString() : dateKey);

        for (const QString &secKey : secs.keys()) {
            // secKey 形如 "c1","c2",...
            QString secNum = secKey;
            secNum.remove(QRegularExpression("[^0-9]")); // 得到 "1","2",...

            // 如果用户选了某些节次，则不在列表里就跳过
            if (!selected.isEmpty() && !selected.contains(secNum))
                continue;

            // 这一节空闲的教室列表
            QJsonArray rooms = secs.value(secKey).toArray();
            for (const QJsonValue &rv : rooms) {
                QString room = rv.toString();
                if (room.isEmpty())
                    continue;

                int row = resultTable->rowCount();
                resultTable->insertRow(row);
                // resultTable->setItem(row, 0, new QTableWidgetItem(displayDate));
                resultTable->setItem(row, 0, new QTableWidgetItem(secNum));
                resultTable->setItem(row, 1, new QTableWidgetItem(room));
                ++count;
            }
        }
    }

    // 7) 更新状态
    if (count == 0) {
        statusLabel->setText("状态：未找到空闲教室");
    } else {
        statusLabel->setText(QString("状态：共找到 %1 条记录").arg(count));
    }
}
