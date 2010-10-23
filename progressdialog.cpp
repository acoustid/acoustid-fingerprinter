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
    connect(fingerprinter, SIGNAL(fileListLoadingStarted()), SLOT(onFileListLoadingStarted()));
    connect(fingerprinter, SIGNAL(fingerprintingStarted(int)), SLOT(onFingerprintingStarted(int)));
    connect(fingerprinter, SIGNAL(currentPathChanged(const QString &)), SLOT(onCurrentPathChanged(const QString &)));
    connect(fingerprinter, SIGNAL(finished()), SLOT(onFinished()));
}

ProgressDialog::~ProgressDialog()
{
}

void ProgressDialog::setupUi()
{
	m_mainStatusLabel = new QLabel(tr("Starting..."));
	m_currentPathLabel = new QLabel();

	m_closeButton = new QPushButton(tr("&Close"));
	connect(m_closeButton, SIGNAL(clicked()), SLOT(close()));

	m_stopButton = new QPushButton(tr("&Stop"));
	connect(m_stopButton, SIGNAL(clicked()), SLOT(stop()));

	m_pauseButton = new QPushButton(tr("&Pause"));
	m_pauseButton->setCheckable(true);
	connect(m_pauseButton, SIGNAL(clicked(bool)), SLOT(togglePause(bool)));

	QDialogButtonBox *buttonBox = new QDialogButtonBox();
	buttonBox->addButton(m_pauseButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(m_stopButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(m_closeButton, QDialogButtonBox::ActionRole);
	m_closeButton->setVisible(false);

	m_progressBar = new QProgressBar();
	m_progressBar->setMinimum(0);
	m_progressBar->setMaximum(0);
	m_progressBar->setFormat(tr("%v of %m"));
	m_progressBar->setTextVisible(false);
    connect(m_fingerprinter, SIGNAL(progress(int)), m_progressBar, SLOT(setValue(int)));

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->addWidget(m_mainStatusLabel);
	mainLayout->addWidget(m_progressBar);
	mainLayout->addWidget(m_currentPathLabel);
	mainLayout->addStretch();
	mainLayout->addWidget(buttonBox);

	setLayout(mainLayout);
	setWindowTitle(tr("Acoustid Fingerprinter"));
	resize(QSize(450, 200));
}

void ProgressDialog::onFileListLoadingStarted()
{
	m_progressBar->setTextVisible(false);
	m_progressBar->setMaximum(0);
	m_progressBar->setValue(0);
	m_mainStatusLabel->setText(tr("Collecting files..."));
}

void ProgressDialog::onFingerprintingStarted(int count)
{
	m_progressBar->setTextVisible(true);
	m_progressBar->setMaximum(count);
	m_progressBar->setValue(0);
	m_mainStatusLabel->setText(tr("Fingerprinting..."));
}

void ProgressDialog::onFinished()
{
	m_mainStatusLabel->setText(tr("Submitted %n fingerprint(s), thank you!", "", m_fingerprinter->submitttedFingerprints()));
	m_closeButton->setVisible(true);
	m_pauseButton->setVisible(false);
	m_stopButton->setVisible(false);
}

void ProgressDialog::onCurrentPathChanged(const QString &path)
{
    QString elidedPath =
        m_currentPathLabel->fontMetrics().elidedText(
            path, Qt::ElideMiddle, m_currentPathLabel->width());
    m_currentPathLabel->setText(elidedPath);
}

void ProgressDialog::setProgress(int value)
{
	m_progressBar->setValue(value);
}

void ProgressDialog::stop()
{
	m_fingerprinter->cancel();
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

