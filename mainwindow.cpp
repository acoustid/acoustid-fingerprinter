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
#include "checkabledirmodel.h"
#include "fingerprinter.h"
#include "mainwindow.h"

static const char *API_KEY_URL = "http://acoustid.org/api-key";

MainWindow::MainWindow()
{
	setupUi();
}

void MainWindow::setupUi()
{

	QTreeView *treeView = new QTreeView();

	m_directoryModel = new CheckableDirModel();
	m_directoryModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
	m_directoryModel->setRootPath("");

	treeView->setModel(m_directoryModel);
	treeView->setHeaderHidden(true);
	treeView->hideColumn(1);
	treeView->hideColumn(2);
	treeView->hideColumn(3);

	const QModelIndex homePathIndex = m_directoryModel->index(QDir::homePath());
	treeView->expand(homePathIndex);
	treeView->selectionModel()->setCurrentIndex(homePathIndex, QItemSelectionModel::ClearAndSelect);
	treeView->scrollTo(homePathIndex);

	QLabel *treeViewLabel = new QLabel(tr("&Select which folders to fingerprint:"));
	treeViewLabel->setBuddy(treeView);

	m_apiKeyEdit = new QLineEdit();
	QPushButton *apiKeyButton = new QPushButton(tr("&Get API key..."));
	connect(apiKeyButton, SIGNAL(clicked()), SLOT(openAcoustidWebsite()));

	QHBoxLayout *apiKeyLayout = new QHBoxLayout();
	apiKeyLayout->addWidget(m_apiKeyEdit);
	apiKeyLayout->addWidget(apiKeyButton);

	QLabel *apiKeyLabel = new QLabel(tr("Your Acoustid &API key:"));
	apiKeyLabel->setBuddy(m_apiKeyEdit);

	QPushButton *fingerprintButton = new QPushButton(tr("&Fingerprint..."));
	connect(fingerprintButton, SIGNAL(clicked()), SLOT(fingerprint()));

	QDialogButtonBox *buttonBox = new QDialogButtonBox();
	buttonBox->addButton(fingerprintButton, QDialogButtonBox::ActionRole);

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->addWidget(apiKeyLabel);
	mainLayout->addLayout(apiKeyLayout);
	mainLayout->addWidget(treeViewLabel);
	mainLayout->addWidget(treeView);
	mainLayout->addWidget(buttonBox);

	QWidget *centralWidget = new QWidget();
	centralWidget->setLayout(mainLayout);
	setCentralWidget(centralWidget);
	setWindowTitle(tr("Acoustid Fingerprinter"));
	resize(QSize(400, 500));
}

void MainWindow::openAcoustidWebsite()
{
	QDesktopServices::openUrl(QUrl::fromPercentEncoding(API_KEY_URL));
}

void MainWindow::fingerprint()
{
	QString apiKey;
	QList<QString> directories;
	if (!validateFields(apiKey, directories)) {
		return;
	}
	Fingerprinter *fingerprinter = new Fingerprinter(apiKey, directories);
    ProgressDialog *progressDialog = new ProgressDialog(this);
    connect(fingerprinter, SIGNAL(mainStatusChanged(const QString &)), progressDialog, SLOT(setMainStatus(const QString &)));
    connect(fingerprinter, SIGNAL(fileListLoaded(int)), progressDialog, SLOT(configureProgressBar(int)));
    connect(fingerprinter, SIGNAL(fileProcessed(int)), progressDialog, SLOT(setProgress(int)));
	fingerprinter->start();
    progressDialog->setModal(true);
    progressDialog->show();
}

bool MainWindow::validateFields(QString &apiKey, QList<QString> &directories)
{
	apiKey = m_apiKeyEdit->text();
	if (apiKey.isEmpty()) {
		QMessageBox::warning(this, tr("Error"),
			tr("Please enter your Acoustid API key. You can get an API key "
			"from the <a href=\"%1\">Acoustid website</a> without any "
			"registration.").arg(API_KEY_URL));
		return false;
	}
	directories = m_directoryModel->selectedDirectories();
	if (directories.isEmpty()) {
		QMessageBox::warning(this, tr("Error"),
			tr("Please select one or more folders with audio files to fingerprint."));
		return false;
	}
	return true;
}

