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
	connect(stopButton, SIGNAL(clicked()), SIGNAL(stop()));

	QDialogButtonBox *buttonBox = new QDialogButtonBox();
	buttonBox->addButton(stopButton, QDialogButtonBox::DestructiveRole);

	m_progressBar = new QProgressBar();
	m_progressBar->setMinimum(0);
	m_progressBar->setMaximum(0);
	m_progressBar->setFormat(tr("%v of %m"));
	m_progressBar->setTextVisible(false);

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->addWidget(m_mainStatusLabel);
	mainLayout->addWidget(m_progressBar);
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

