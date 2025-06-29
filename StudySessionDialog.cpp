#include "StudySessionDialog.h"
#include "DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QDebug>
#include <QEvent>
#include <QMovie> // 引入 QMovie

StudySessionDialog::StudySessionDialog(QWidget *parent)
    : QDialog(parent), timer(new QTimer(this)), animation(nullptr) // 初始化 animation 为 nullptr
{
    setWindowTitle("自习中...");
    resize(300, 400);

    timerLabel = new QLabel("00:00:00", this);
    timerLabel->setAlignment(Qt::AlignCenter);
    timerLabel->setStyleSheet("font-size: 24px;");

    gifLabel = new QLabel(this);
    // 在这里加载您的 GIF。请确保 ":/images/Ralsei_emote.gif" 存在于您的 .qrc 文件中
    // 假设您已经将 Ralsei_emote.gif 添加到了资源的 /images 路径下
    animation = new QMovie(":/images/Ralsei_study_dialog.gif", QByteArray(), this);
    if (!animation->isValid()) {
        qDebug() << "错误：无法加载 GIF 文件: :/images/lancer.gif";
        // 如果 GIF 加载失败，可以添加备用方案或错误处理
    } else {
        gifLabel->setMovie(animation);
        animation->start(); // 开始播放 GIF 动画
    }

    gifLabel->setAlignment(Qt::AlignCenter);
    gifLabel->setFixedSize(200, 200);
    // 之前用于 QPixmap 鼠标悬停效果的事件过滤器已不再需要用于 GIF 播放
    // gifLabel->installEventFilter(this); // 如果没有其他过滤需求，可以移除此行

    endButton = new QPushButton("结束自习", this);
    connect(endButton, &QPushButton::clicked,
            this, &StudySessionDialog::onEndSessionClicked);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    mainLayout->addWidget(timerLabel, 0, Qt::AlignHCenter);
    mainLayout->addWidget(gifLabel, 0, Qt::AlignHCenter);
    mainLayout->addWidget(endButton, 0, Qt::AlignHCenter);

    setLayout(mainLayout);

    startTime = QTime::currentTime(); // 初始化为当前时间
    timerLabel->setText("00:00:00");

    connect(timer, &QTimer::timeout,
            this, &StudySessionDialog::updateTimer);
    timer->start(1000);
}

void StudySessionDialog::updateTimer()
{
    int elapsed = startTime.secsTo(QTime::currentTime());
    int hours = elapsed / 3600;
    int minutes = (elapsed % 3600) / 60;
    int seconds = elapsed % 60;
    timerLabel->setText(
        QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
        );
}

void StudySessionDialog::onEndSessionClicked()
{
    timer->stop();
    // 使用 state() 函数检查 QMovie 的状态
    if (animation && animation->state() == QMovie::Running) {
        animation->stop(); // 在会话结束时停止 GIF 动画
    }
    QDateTime endDateTime = QDateTime::currentDateTime();
    QDateTime startDateTime(QDate::currentDate(), startTime);

    saveStudySession(startDateTime, endDateTime);

    accept();
}

void StudySessionDialog::saveStudySession(const QDateTime &start, const QDateTime &end)
{
    int durationSeconds = start.secsTo(end);
    qDebug() << "自习开始:" << start.toString(Qt::ISODate)
             << " 结束:" << end.toString(Qt::ISODate)
             << " 时长(秒):" << durationSeconds;

    // 假设 DatabaseManager::instance().addStudySession 方法存在
    DatabaseManager::instance().addStudySession(start, end, durationSeconds);
}

// eventFilter 已修改，移除了 QPixmap 悬停逻辑
bool StudySessionDialog::eventFilter(QObject *obj, QEvent *event)
{
    // 如果您需要 GIF 的自定义悬停效果，那将需要不同的方法，
    // 例如，切换 QMovie 对象或操纵 QMovie 的速度/帧。
    return QDialog::eventFilter(obj, event);
}

void StudySessionDialog::startSession()
{
    // 每次开始自习时，将计时器重置为当前时间，并更新显示
    startTime = QTime::currentTime();
    timerLabel->setText("00:00:00");

    if (!timer->isActive())
        timer->start(1000);

    // 确保在会话开始时 GIF 动画开始或恢复
    // 使用 state() 函数检查 QMovie 的状态
    if (animation && animation->state() != QMovie::Running) {
        animation->start();
    }
}
