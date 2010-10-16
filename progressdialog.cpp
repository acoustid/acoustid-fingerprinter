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
#include <QCloseEvent>
#include "progressdialog.h"

ProgressDialog::ProgressDialog(QWidget *parent, Fingerprinter *fingerprinter)
	: QDialog(parent), m_fingerprinter(fingerprinter)
{
	setupUi();
    connect(fingerprinter, SIGNAL(mainStatusChanged(const QString &)), SLOT(setMainStatus(const QString &)));
    connect(fingerprinter, SIGNAL(fileListLoaded(int)), SLOT(configureProgressBar(int)));
    connect(fingerprinter, SIGNAL(fileProcessed(int)), SLOT(setProgress(int)));
    connect(fingerprinter, SIGNAL(finished()), SLOT(close()));
}

ProgressDialog::~ProgressDialog()
{
}

void ProgressDialog::setupUi()
{
	m_mainStatusLabel = new QLabel(tr("Starting..."));

	m_stopButton = new QPushButton(tr("&Stop"));
	connect(m_stopButton, SIGNAL(clicked()), SLOT(stop()));

	m_pauseButton = new QPushButton(tr("&Pause"));
	m_pauseButton->setCheckable(true);
	connect(m_pauseButton, SIGNAL(clicked(bool)), SLOT(togglePause(bool)));

	QDialogButtonBox *buttonBox = new QDialogButtonBox();
	buttonBox->addButton(m_pauseButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(m_stopButton, QDialogButtonBox::ActionRole);

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

void ProgressDialog::stop()
{
	m_fingerprinter->stop();
	m_pauseButton->setEnabled(false);
	m_stopButton->setEnabled(false);
}

void ProgressDialog::closeEvent(QCloseEvent *event)
{
	if (!m_fingerprinter->isFinished()) {
		stop();
		event->ignore();
	}
	else {
		event->accept();
	}
}

void ProgressDialog::togglePause(bool checked)
{
	if (checked) {
		m_fingerprinter->pause();
	}
	else {
		m_fingerprinter->resume();
	}
}

