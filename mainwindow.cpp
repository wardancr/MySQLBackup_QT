#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>

/*
    Project depends on QuaZip

    SMTP Client with SSL/TLS
    libraries/smtp in source tree
    https://www.codeproject.com/Articles/98355/SMTP-Client-with-SSL-TLS
*/


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // create objects for the labels and progress bar
    statusLabelOS = new QLabel(this);
    statusLabelConfig = new QLabel(this);
    statusLabelConfigName = new QLabel(this);
    statusLabelScheduler = new QLabel(this);
    statusProgressBar = new QProgressBar(this);
    // set text for the labels
    statusLabelConfig->setText("Config File:");
    statusLabelConfigName->setText("None");
    statusLabelScheduler->setText("Scheduler: Stopped");
    // make progress bar text invisible
    statusProgressBar->setMinimum(0);
    statusProgressBar->setMaximum(1);
    // add the controls to the status bar
    ui->statusBar->addPermanentWidget(statusLabelOS);
    ui->statusBar->addPermanentWidget(statusLabelConfig);
    ui->statusBar->addPermanentWidget(statusLabelConfigName);
    ui->statusBar->addPermanentWidget(statusLabelScheduler);
    ui->statusBar->addPermanentWidget(statusProgressBar,1);
    // What system are we running on?
    QSysInfo info;
    if (info.productType() == "windows") {
        statusLabelOS->setText("OS: " + info.prettyProductName());
        ui->rbWindows->setChecked(true);
    }
    else {
        statusLabelOS->setText("OS: " + info.prettyProductName());
        ui->rbLinux->setChecked(true);
    }
    QCoreApplication::setOrganizationName("CDurbin");
    QCoreApplication::setOrganizationDomain("cdcomputersys.com");
    QCoreApplication::setApplicationName("MySQL Backup");

    //Set mandatory flag and group name
    ui->tbMySQLHostName->setProperty("mandatory","mysql");
    ui->tbMySQLPort->setProperty("mandatory","mysql");
    ui->tbMySQLUserName->setProperty("mandatory","mysql");
    ui->tbMySQLPassword->setProperty("mandatory","mysql");

    ui->tbMySQLDumpLocation->setProperty("mandatory","mysqldump");
    ui->tbMySQLBackupLocation->setProperty("mandatory","mysqldump");
    ui->tbMySQLDaysToSave->setProperty("mandatory","mysqldump");

    ui->tbSMTPServerName->setProperty("mandatory","smtp");
    ui->tbSMTPServerPort->setProperty("mandatory","smtp");
    //ui->tbSMTPUserName->setProperty("mandatory","smtp");
    //ui->tbSMTPPassword->setProperty("mandatory","smtp");
    ui->tbSMTPEMailAddress->setProperty("mandatory","smtp");
    ui->tbSMTPFromAddress->setProperty("mandatory","smtp");

}

MainWindow::~MainWindow()
{
    delete ui;
}

/**********************************************************************
    Get Database Names
**********************************************************************/
void MainWindow::on_buGetDatabaseNames_clicked() {    

    if(errorProvider("mysql")){
        QMessageBox::critical(this, "Error", "Please enter the MySQL server information or Open a config file before clicking on Get Database Names.", QMessageBox::Ok);
        return;
    }
    //Clear any exisiting items out
    ui->clbDatabases->clear();
    // Everything appears to be here so we can try a connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName(ui->tbMySQLHostName->text());
    db.setDatabaseName("mysql");
    db.setUserName(ui->tbMySQLUserName->text());
    db.setPassword(ui->tbMySQLPassword->text());
    db.setPort(ui->tbMySQLPort->text().toInt());
    // Exceptions only in certain QT modules???? Ughhh.
    if(!db.open()) {
        QMessageBox::critical(this, "Error", db.lastError().text(), QMessageBox::Ok);
        return;
    }
    QSqlQuery query("SHOW DATABASES;",db);
    while(query.next()) {
        ui->clbDatabases->addItem(query.value(0).toString());
    }
    // Close the database connection
    db.close();
}

/**********************************************************************
    Select All Databases Clicked
**********************************************************************/
void MainWindow::on_cbSelectDatabases_clicked() {
    if(ui->cbSelectDatabases->isChecked()) {
        ui->clbDatabases->selectAll();
    }
    else {
        ui->clbDatabases->clearSelection();
    }
}

/**********************************************************************
    Exit the Application
**********************************************************************/
void MainWindow::on_actionQuit_triggered() {
    if(QMessageBox::question(this, "Exit Application", "Exit MySQL Backup?", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
            QApplication::quit();
    }
}

/**********************************************************************
    Find MySQL Dump
**********************************************************************/
void MainWindow::on_buMySQLDumpLocation_clicked() {
   ui->tbMySQLDumpLocation->setText(QFileDialog::getOpenFileName(this,tr("Find mysqldump"), "/usr/bin", tr("MySQL Dump (mysqldump)")));
}

/**********************************************************************
    Set Backup Directory
**********************************************************************/
void MainWindow::on_buMySQLBackupSaveLocation_clicked() {
    QDir dir;
    QString directory = QFileDialog::getExistingDirectory(this,tr("Backup Save Directory"),dir.absolutePath());
    if(!directory.isNull()) {
        ui->tbMySQLBackupLocation->setText(directory);
    }
}

/**********************************************************************
    Remove SQL Dump File Checkbox
**********************************************************************/
void MainWindow::on_cbRemoveSQLDumpFile_clicked() {
    if (ui->cbRemoveSQLDumpFile->isChecked() && !ui->cbCompressBackup->isChecked()) {
        QMessageBox::information(this,"Warning", "Checking this and not checking Compress Backup WILL result in your backup file being deleted!\nThis is designed to let you keep the original dump file and the compressed backup.",QMessageBox::Ok);
    }
}

/**********************************************************************
    Save Clicked
**********************************************************************/
void MainWindow::on_actionSave_triggered() {
    if(!configFile.isNull() && !configFile.isEmpty()) {
        SaveConfig(configFile);
    }
}

/**********************************************************************
    Save As Clicked
**********************************************************************/
void MainWindow::on_actionSave_As_triggered() {
    //Try to find the default config directory. If not create one.
    QDir dir;
    if(!QDir(dir.absolutePath() + "/config").exists()) {
        if(!QDir().mkdir(dir.absolutePath() + "/config")) {
            QMessageBox::critical(this,"Error","Unable to create default config directory!");
        }
    }
    //Get a filename and send it to the save config function
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As File"), dir.absolutePath() + "/config", tr("conf (*.conf)"));
    if(!fileName.isNull() && !fileName.isEmpty()) {
        if(SaveConfig(fileName)){
            statusLabelConfigName->setText(QFileInfo(fileName).fileName());
            ui->actionSave->setEnabled(true);
            configFile = fileName;
        }
    }
}

/**********************************************************************
    Save A Config File
**********************************************************************/
bool MainWindow::SaveConfig(QString fileName) {
    if(QFileInfo(fileName).suffix() != "conf") {
        fileName += ".conf";
    }
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)){
        QMessageBox::critical(this,"Error","Unable to save the config file!");
        return false;
    }
    else {
        file.close();
    }
    // Encrypt the passwords when we store them. We base the key off a sha256 hash of servername, username and server port.
    QDataStream dataStream(QCryptographicHash::hash(ui->tbMySQLHostName->text().toUtf8() + ui->tbMySQLPort->text().toUtf8() + ui->tbMySQLUserName->text().toUtf8(),QCryptographicHash::Sha256));
    quint64 key;
    dataStream >> key;
    SimpleCrypt crypto(key);
    QSettings settings(fileName,QSettings::IniFormat);
    settings.beginGroup("generalsettings");
        settings.setValue("configversion",QString::fromStdString(VERSION_STRING));
            if(ui->rbWindows->isChecked()) {
                settings.setValue("configos","windows");
            }
            else{
                settings.setValue("configos","linux");
            }
        settings.setValue("compressbackup", ui->cbCompressBackup->isChecked()?"true":"false" );
        settings.setValue("removedumpfile", ui->cbRemoveSQLDumpFile->isChecked()?"true":"false" );
        settings.setValue("usedbdirs", ui->cbUseDBDirs->isChecked()?"true":"false" );
    settings.endGroup();
    settings.beginGroup("dbserverinfo");
        settings.setValue("hostname", ui->tbMySQLHostName->text());
        settings.setValue("port", ui->tbMySQLPort->text());
        settings.setValue("username", ui->tbMySQLUserName->text());
        settings.setValue("password", crypto.encryptToString(ui->tbMySQLPassword->text().toUtf8()));
    settings.endGroup();
    settings.beginGroup("mysqldump");
        settings.setValue("mysqldumplocation", ui->tbMySQLDumpLocation->text());
        settings.setValue("mysqladditionaloptions", ui->tbAdditionalOptions->text());
        settings.setValue("backupsavelocation", ui->tbMySQLBackupLocation->text());
        settings.setValue("daystosave", ui->tbMySQLDaysToSave->text());
    settings.endGroup();
    settings.beginGroup("databases");
        QString selected;
        QString notselected;
        for(int i = 0; i < ui->clbDatabases->count(); ++i) {
            // May be a better way to do this?
            if(ui->clbDatabases->item(i)->isSelected()) {
                selected += ui->clbDatabases->item(i)->text();
                selected += ",";
            }
            else {
                notselected += ui->clbDatabases->item(i)->text();
                notselected += ",";
            }
        }
        settings.setValue("selected", selected.left(selected.length()-1));
        settings.setValue("notselected", notselected.left(notselected.length()-1));
    settings.endGroup();
    settings.beginGroup("smtpserver");
        settings.setValue("smtpserver", ui->tbSMTPServerName->text());
        settings.setValue("smtpport", ui->tbSMTPServerPort->text());
        settings.setValue("smtpusername", ui->tbSMTPUserName->text());
        settings.setValue("smtppassword", crypto.encryptToString(ui->tbSMTPPassword->text().toUtf8()));
        settings.setValue("smtpemailaddress", ui->tbSMTPEMailAddress->text());
        settings.setValue("smtpfromaddress", ui->tbSMTPFromAddress->text());
        settings.setValue("sendmail", ui->cbSendEmail->isChecked()?"true":"false" );
        if(ui->rbSMTPNone->isChecked()) {
            settings.setValue("smtpsecurity", "none" );
        }
        else if(ui->rbSMTPTLS->isChecked()) {
            settings.setValue("smtpsecurity", "tls" );
        }
        else {
            settings.setValue("smtpsecurity", "ssl" );
        }
    settings.endGroup();
    if (errorProvider("all")) {
        QMessageBox::information(this,"File Saved","The config file was saved, but it was missing mandatory fields. This may cause issues when trying to run the configuration.");
    }
    else {
        QMessageBox::information(this,"File Saved","The config file was saved!");
    }

    return true;
}

/**********************************************************************
    Open A Config File
**********************************************************************/
void MainWindow::on_actionOpen_triggered() {
    //Try to find the default config directory. If not create one.
    QDir dir;
    if(!QDir(dir.absolutePath() + "/config").exists()) {
        if(!QDir().mkdir(dir.absolutePath() + "/config")) {
            QMessageBox::critical(this,"Error","Unable to create default config directory!");
        }
    }
    //Get a filename and send it to the save config function
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), dir.absolutePath() + "/config", tr("conf (*.conf)"));
    if(!fileName.isNull() && !fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly | QFile::Text)){
            QMessageBox::critical(this,"Error","Unable to open the config file!");
            return;
        }
        else {
            file.close();
        }
        QSettings settings(fileName,QSettings::IniFormat);
        QDataStream dataStream(QCryptographicHash::hash(settings.value("dbserverinfo/hostname").toByteArray() + settings.value("dbserverinfo/port").toByteArray() + settings.value("dbserverinfo/username").toByteArray(),QCryptographicHash::Sha256));
        quint64 key;
        dataStream >> key;
        SimpleCrypt crypto(key);

        ui->tbMySQLHostName->setText(settings.value("dbserverinfo/hostname").toString());
        ui->tbMySQLPort->setText(settings.value("dbserverinfo/port").toString());
        ui->tbMySQLUserName->setText(settings.value("dbserverinfo/username").toString());
        ui->tbMySQLPassword->setText(crypto.decryptToString(settings.value("dbserverinfo/password").toString()));
        if(settings.value("generalsettings/configos").toString() == "linux") {
            ui->rbLinux->setChecked(true);
        }
        else {
            ui->rbWindows->setChecked(true);
        }
        if(settings.value("generalsettings/compressbackup") == "true") {
            ui->cbCompressBackup->setChecked(true);
        }
        else {
            ui->cbCompressBackup->setChecked(false);
        }
        if(settings.value("generalsettings/removedumpfile") == "true") {
            ui->cbRemoveSQLDumpFile->setChecked(true);
        }
        else {
            ui->cbRemoveSQLDumpFile->setChecked(false);
        }
        if(settings.value("generalsettings/usedbdirs") == "true") {
            ui->cbUseDBDirs->setChecked(true);
        }
        else {
            ui->cbUseDBDirs->setChecked(false);
        }
        ui->tbMySQLBackupLocation->setText(settings.value("mysqldump/backupsavelocation").toString());
        ui->tbMySQLDumpLocation->setText(settings.value("mysqldump/mysqldumplocation").toString());
        ui->tbMySQLDaysToSave->setText(settings.value("mysqldump/daystosave").toString());
        ui->tbAdditionalOptions->setText(settings.value("mysqldump/mysqladditionaloptions").toString());

        ui->clbDatabases->clear();
        QStringList selist = settings.value("databases/selected").toString().split(',');
        for (int i = 0; i < selist.size(); ++i) {
            ui->clbDatabases->addItem(selist.value(i));
            ui->clbDatabases->item(i)->setSelected(true);
        }
        QStringList uslist = settings.value("databases/notselected").toString().split(',');
        for (int i = 0; i < uslist.size(); ++i) {
            ui->clbDatabases->addItem(uslist.value(i));
        }
        if(settings.value("smtpserver/sendmail") == "true") {
            ui->cbSendEmail->setChecked(true);
            on_cbSendEmail_clicked();
        }
        else {
            ui->cbSendEmail->setChecked(false);
            on_cbSendEmail_clicked();
        }
        ui->tbSMTPServerName->setText(settings.value("smtpserver/smtpserver").toString());
        ui->tbSMTPServerPort->setText(settings.value("smtpserver/smtpport").toString());
        ui->tbSMTPUserName->setText(settings.value("smtpserver/smtpusername").toString());
        ui->tbSMTPEMailAddress->setText(settings.value("smtpserver/smtpemailaddress").toString());
        ui->tbSMTPFromAddress->setText(settings.value("smtpserver/smtpfromaddress").toString());
        ui->tbSMTPPassword->setText(crypto.decryptToString(settings.value("smtpserver/smtppassword").toString()));
        if(settings.value("smtpserver/smtpsecurity").toString() == "none") {
           ui->rbSMTPNone->setChecked(true);
        }
        else if(settings.value("smtpserver/smtpsecurity").toString() == "tls") {
            ui->rbSMTPTLS->setChecked(true);
        }
        else {
            ui->rbSMTPSSL->setChecked(true);
        }
        statusLabelConfigName->setText(QFileInfo(fileName).fileName());
        ui->actionSave->setEnabled(true);
        configFile = fileName;
        // Check and clear any unwanted error states
        errorProvider("all");
    }
}

/**********************************************************************
    New Config File
**********************************************************************/
void MainWindow::on_actionNew_triggered() {
    QSysInfo info;
    ui->actionSave->setEnabled(false);
    statusLabelConfigName->setText("None");
    configFile = "";
    ui->tbMySQLHostName->setText("");
    ui->tbMySQLPort->setText("3306");
    ui->tbMySQLUserName->setText("");
    ui->tbMySQLPassword->setText("");
    ui->clbDatabases->clear();
    if (info.productType() == "windows") {
        ui->rbWindows->setChecked(true);
    }
    else {
        ui->rbLinux->setChecked(true);
    }
    errorProvider("clear");
}

/**********************************************************************
    Toggle Send Mail
**********************************************************************/
void MainWindow::on_cbSendEmail_clicked() {
    if(ui->cbSendEmail->isChecked()) {
        ui->tbSMTPServerName->setEnabled(true);
        ui->tbSMTPServerPort->setEnabled(true);
        ui->tbSMTPUserName->setEnabled(true);
        ui->tbSMTPPassword->setEnabled(true);
        ui->tbSMTPEMailAddress->setEnabled(true);
        ui->tbSMTPFromAddress->setEnabled(true);
        ui->buTestMail->setEnabled(true);
    }
    else {
        ui->tbSMTPServerName->setEnabled(false);
        ui->tbSMTPServerPort->setEnabled(false);
        ui->tbSMTPUserName->setEnabled(false);
        ui->tbSMTPPassword->setEnabled(false);
        ui->tbSMTPEMailAddress->setEnabled(false);
        ui->tbSMTPFromAddress->setEnabled(false);
        ui->buTestMail->setEnabled(false);
        errorProvider("smtp");
    }
}

/**********************************************************************
    About
**********************************************************************/
void MainWindow::on_actionAbout_triggered() {
    QMessageBox::information(this,"About",QWidget::windowTitle() + "\nCopyright 2018 " + QCoreApplication::organizationName() + "\n" +QCoreApplication::organizationDomain(),QMessageBox::Ok);
}

/**********************************************************************
    Test The Current Settings
**********************************************************************/
void MainWindow::on_buTestConfig_clicked() {
    // Check all required linedits
    if(errorProvider("all")){
        QMessageBox::critical(this, "Error", "Please enter the required information or Open a config file before clicking on Test Configuration.", QMessageBox::Ok);
        return;
    }
    //Try to find the default tmp directory. If not create one.
    QDir dir;
    if(!QDir(dir.absolutePath() + "/tmp").exists()) {
        if(!QDir().mkdir(dir.absolutePath() + "/tmp")) {
            QMessageBox::critical(this,"Error","Unable to create default tmp directory!");
        }
    }









}

/**********************************************************************
    Test The Current E-Mail Settings
**********************************************************************/
void MainWindow::on_buTestMail_clicked() {
    // Check all required linedits
    if(errorProvider("smtp")){
        QMessageBox::critical(this, "Error", "Please enter the required information or Open a config file before clicking on Test E-Mail.", QMessageBox::Ok);
        return;
    }    
    try {
        CSmtp mail;
        mail.SetSMTPServer(ui->tbSMTPServerName->text().toUtf8(),ui->tbSMTPServerPort->text().toShort());
        if (!ui->tbSMTPUserName->text().isEmpty() && !ui->tbSMTPUserName->text().isNull()) {
            mail.SetLogin(ui->tbSMTPUserName->text().toUtf8());
        }
        if (!ui->tbSMTPPassword->text().isEmpty() && !ui->tbSMTPUserName->text().isNull()) {
            mail.SetPassword(ui->tbSMTPPassword->text().toUtf8());
        }
        /*  NO_SECURITY,
            USE_TLS,
            USE_SSL,
            DO_NOT_SET
        */
        if(ui->rbSMTPNone->isChecked()) {
            mail.SetSecurityType(NO_SECURITY);
        }
        else if (ui->rbSMTPTLS->isChecked()) {
            mail.SetSecurityType(USE_TLS);
        }
        else {
            mail.SetSecurityType(USE_SSL);
        }
        mail.SetSenderName("MySQLBackup");
        if (!ui->tbSMTPFromAddress->text().isEmpty() && !ui->tbSMTPFromAddress->text().isNull()) {
            mail.SetSenderMail(ui->tbSMTPFromAddress->text().toUtf8());
            mail.SetReplyTo(ui->tbSMTPFromAddress->text().toUtf8());
        }
        mail.SetSubject("MySQL Backup Test Message");
        if (!ui->tbSMTPEMailAddress->text().isEmpty() && !ui->tbSMTPEMailAddress->text().isNull()) {
            mail.AddRecipient(ui->tbSMTPEMailAddress->text().toUtf8());
        }
        mail.SetXPriority(XPRIORITY_NORMAL);
        mail.SetXMailer("MySQL Backup v" + QString::fromStdString(VERSION_STRING).toUtf8());
        mail.AddMsgLine("Hello, this is a test message from the MySQL Backup Program.");
        mail.Send();
    }
    catch(ECSmtp e) {
        QMessageBox::critical(this,"SMTP Error",e.GetErrorText().c_str());
        return;
    }
QMessageBox::information(this,"SMTP Send","Mail was send successfully.");
}

/**********************************************************************
    Error Provider
**********************************************************************/
bool MainWindow::errorProvider(QString groupname) {
    QString str;
    int first = 9999;
    bool error = false;
    // Get a list of lineedits on the centralWidget
    QList<QLineEdit*> line_edits = centralWidget()->findChildren<QLineEdit*>();
    // Iterate through the list and check to see if the mandatory property is the same as the groupname that is sent.
    // If so, change the stylesheet, display the icon.
    for(int i = 0; i < line_edits.count(); ++i) {
        if (groupname == "clear" && line_edits[i]->property("HasError") == true) {
            line_edits[i]->setStyleSheet("");
            // Delete the error symbol from the form
            centralWidget()->findChild<QLabel *>(line_edits[i]->property("ErrorWidget").toString())->deleteLater();
            line_edits[i]->setProperty("HasError",false);
        }
        else if(line_edits[i]->isEnabled() && line_edits[i]->property("mandatory").isValid() && (line_edits[i]->property("mandatory") == groupname || groupname == "all")) {
            if(line_edits[i]->text() == "" || line_edits[i]->text().isNull()) {
                if(line_edits[i]->property("HasError") == false || line_edits[i]->property("HasError").isNull()) {
                    line_edits[i]->setStyleSheet("color: #FF0000;");
                    QPixmap image(":/mysqlbackup_qt/icons/Icons/exclamation-red-icon.png");
                    QLabel *errorLabel = new QLabel();
                    errorLabel->setObjectName(line_edits[i]->objectName() + "_errorlabel");
                    errorLabel->setPixmap(image);
                    errorLabel->setGeometry(line_edits[i]->x() + line_edits[i]->width() + 5,line_edits[i]->y() + 5,16,16);
                    errorLabel->setParent(line_edits[i]->parentWidget());
                    errorLabel->show();
                    line_edits[i]->setProperty("ErrorWidget",errorLabel->objectName());
                    line_edits[i]->setProperty("HasError",true);
                    // Looks like at least one item failed so set error to true
                    error = true;
                    // Is first set to default? If so, assign it the current value
                    if (first == 9999) {
                        first = i;
                    }
                }
                else if(line_edits[i]->property("HasError") == true) {
                    // So we've already processed these lineedits and at least one is still in error
                    error = true;
                    // Is first set to default? If so, assign it the current value
                    if (first == 9999) {
                        first = i;
                    }
                }
            }
            else if(line_edits[i]->property("HasError").toBool()) {
                    line_edits[i]->setStyleSheet("");
                    // Delete the error symbol from the form
                    centralWidget()->findChild<QLabel *>(line_edits[i]->property("ErrorWidget").toString())->deleteLater();
                    line_edits[i]->setProperty("HasError",false);
            }
        }
    }
    //Set focus to the first item we found with an error
    if(error && first !=9999) {
        line_edits[first]->setFocus();
    }
    return error;
}