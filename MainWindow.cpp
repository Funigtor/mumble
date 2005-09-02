/* Copyright (C) 2005, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QMenuBar>
#include <QInputDialog>
#include <QMessageBox>
#include "MainWindow.h"

MainWindow *g_mwMainWindow;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
	setupGui();

	connect(g_shServer, SIGNAL(connected()), this, SLOT(serverConnected()));
	connect(g_shServer, SIGNAL(disconnected()), this, SLOT(serverDisconnected()));
}

void MainWindow::setupGui()  {
	QMenu *qmServer, *qmPlayer, *qmAudio, *qmHelp;

	m_qlwPlayers = new QListWidget(this);
	setCentralWidget(m_qlwPlayers);

	qmServer = new QMenu("&Server", this);
	qmPlayer = new QMenu("&Player", this);
	qmAudio = new QMenu("&Audio", this);
	qmHelp = new QMenu("&Help", this);

	m_qaServerConnect=new QAction("&Connect", this);
	m_qaServerDisconnect=new QAction("&Disconnect", this);
	m_qaServerStats=new QAction("&Stats", this);
	m_qaServerConnect->setObjectName("ServerConnect");
	m_qaServerDisconnect->setObjectName("ServerDisconnect");
	m_qaServerStats->setObjectName("ServerStats");
	m_qaServerDisconnect->setEnabled(FALSE);
	m_qaServerStats->setEnabled(FALSE);

	qmServer->addAction(m_qaServerConnect);
	qmServer->addAction(m_qaServerDisconnect);
	qmServer->addAction(m_qaServerStats);


	m_qaPlayerKick=new QAction("&Kick", this);
	m_qaPlayerMute=new QAction("&Mute", this);
	m_qaPlayerDeaf=new QAction("&Deafen", this);
	m_qaPlayerKick->setObjectName("PlayerKick");
	m_qaPlayerMute->setObjectName("PlayerMute");
	m_qaPlayerDeaf->setObjectName("PlayerDeaf");
	m_qaPlayerKick->setEnabled(FALSE);
	m_qaPlayerMute->setCheckable(TRUE);
	m_qaPlayerMute->setEnabled(FALSE);
	m_qaPlayerDeaf->setCheckable(TRUE);
	m_qaPlayerDeaf->setEnabled(FALSE);

	qmPlayer->addAction(m_qaPlayerKick);
	qmPlayer->addAction(m_qaPlayerMute);
	qmPlayer->addAction(m_qaPlayerDeaf);


	m_qaAudioConfig=new QAction("&Config", this);
	m_qaAudioReset=new QAction("&Reset", this);
	m_qaAudioConfig->setObjectName("AudioConfig");
	m_qaAudioReset->setObjectName("AudioReset");

	qmAudio->addAction(m_qaAudioConfig);
	qmAudio->addAction(m_qaAudioReset);

	m_qaHelpAbout=new QAction("&About", this);
	m_qaHelpAbout->setObjectName("HelpAbout");
	m_qaHelpAboutQt=new QAction("&About QT", this);
	m_qaHelpAboutQt->setObjectName("HelpAboutQt");

	qmHelp->addAction(m_qaHelpAbout);
	qmHelp->addAction(m_qaHelpAboutQt);

	menuBar()->addMenu(qmServer);
	menuBar()->addMenu(qmPlayer);
	menuBar()->addMenu(qmAudio);
	menuBar()->addMenu(qmHelp);

    QMetaObject::connectSlotsByName(this);
}

void MainWindow::on_ServerConnect_triggered()
{
	m_qaServerConnect->setEnabled(FALSE);

	QString server = QInputDialog::getText(this, "Server Address", "Addr", QLineEdit::Normal, "128.39.114.2");
	QString user = QInputDialog::getText(this, "Username", "Uname");

	g_shServer->setConnectionInfo(server, user);
	g_shServer->start();
}

void MainWindow::on_ServerDisconnect_triggered()
{
	g_shServer->disconnect();
	m_qaServerDisconnect->setEnabled(FALSE);
}

void MainWindow::on_HelpAbout_triggered()
{
	AboutDialog adAbout(this);
	adAbout.exec();
}

void MainWindow::on_HelpAboutQt_triggered()
{
	QMessageBox::aboutQt(this, "About Qt");
}

void MainWindow::serverConnected()
{
	m_qaServerDisconnect->setEnabled(TRUE);
}

void MainWindow::serverDisconnected()
{
	m_qaServerConnect->setEnabled(TRUE);
	QMapIterator<short, QListWidgetItem *> iItems(m_qmPlayers);
	while (iItems.hasNext()) {
		iItems.next();
		Player *p=m_qmPlayerWidgets.take(iItems.value());
		delete p;
		delete iItems.value();
	}
	m_qmPlayers.clear();
}

void MainWindow::customEvent(QEvent *evt) {
	if (evt->type() != SERVERSEND_EVENT)
		return;

	ServerHandlerMessageEvent *shme=static_cast<ServerHandlerMessageEvent *>(evt);

	Message *mMsg = Message::networkToMessage(shme->qbaMsg);
	if (mMsg) {
		mMsg->process(NULL);
		delete mMsg;
	}
}

void MessageServerJoin::process(Connection *) {
	QListWidgetItem *item = new QListWidgetItem(m_qsPlayerName, g_mwMainWindow->m_qlwPlayers);
	Player *p = new Player();
	p->m_qsName = m_qsPlayerName;
	p->m_sId = m_sPlayerId;
	item->setData(Qt::UserRole, p);

	g_mwMainWindow->m_qmPlayerWidgets[item]=p;
	g_mwMainWindow->m_qmPlayers[m_sPlayerId]=item;
}

void MessageServerLeave::process(Connection *) {
	if (g_mwMainWindow->m_qmPlayers.contains(m_sPlayerId)) {
		QListWidgetItem *item=g_mwMainWindow->m_qmPlayers.take(m_sPlayerId);
		Player *p=g_mwMainWindow->m_qmPlayerWidgets.take(item);

		delete item;
		delete p;
	}
}

void MessageSpeex::process(Connection *) {
}
