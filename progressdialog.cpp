#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QTreeView>
#include <QPushButton>
#include <QDebug>
#include "progressdialog.h"

ProgressDialog::ProgressDialog(QWidget *parent)
	: QDialog(parent)
{
	setupUi();
}

ProgressDialog::~ProgressDialog()
{
}

void ProgressDialog::setupUi()
{
	m_mainStatusLabel = new QLabel(tr("Starting..."));

	QPushButton *stopButton = new QPushButton(tr("&Stop"));
	connect(stopButton, SIGNAL(clicked()), SIGNAL(stopClicked()));

	m_pauseButton = new QPushButton(tr("&Pause"));
	m_pauseButton->setCheckable(true);
	connect(m_pauseButton, SIGNAL(clicked(bool)), SLOT(togglePause(bool)));

	QDialogButtonBox *buttonBox = new QDialogButtonBox();
	buttonBox->addButton(m_pauseButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(stopButton, QDialogButtonBox::ActionRole);

	m_progressBar = new QProgressBar();
	m_progressBar->setMinimum(0);
	m_progressBar->setMaximum(0);
	m_progressBar->setFormat(tr("%v of %m"));
	m_progressBar->setTextVisible(false);

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->addWidget(m_mainStatusLabel);
	mainLayout->addWidget(m_progressBar);
	mainLayout->addStretch();
	mainLayout->addWidget(buttonBox);

	setLayout(mainLayout);
	setWindowTitle(tr("Acoustid Fingerprinter"));
}

void ProgressDialog::configureProgressBar(int maximum)
{
	m_progressBar->setTextVisible(true);
	m_progressBar->setMaximum(maximum);
	m_progressBar->setValue(0);
}

void ProgressDialog::setMainStatus(const QString &message)
{
	m_mainStatusLabel->setText(message);
}

void ProgressDialog::setProgress(int value)
{
	m_progressBar->setValue(value);
}

void ProgressDialog::togglePause(bool checked)
{
	qDebug() << "togglePause" << checked;
	if (checked) {
		m_pauseButton->setText(tr("&Resume"));
		emit pauseClicked();
	}
	else {
		m_pauseButton->setText(tr("&Pause"));
		emit resumeClicked();
	}
}

