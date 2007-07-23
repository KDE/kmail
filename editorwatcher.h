/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef KMAIL_EDITORWATCHER_H
#define KMAIL_EDITORWATCHER_H

#include <kurl.h>

#include <qdatetime.h>
#include <qobject.h>
#include <qtimer.h>

class K3Process;

namespace KMail {

/**
  Starts an editor for the given URL and emits an signal when
  editing has been finished. Both, the editor process as well
  as the edited file are watched to work with as many as possible
  editors.
*/
class EditorWatcher : public QObject
{
  Q_OBJECT
  public:
    EditorWatcher( const KUrl &url, const QString &mimeType, bool openWith, QObject *parent = 0 );
    bool start();
    bool fileChanged() const { return mFileModified; }
  signals:
    void editDone( KMail::EditorWatcher* watcher );

  private slots:
    void editorExited();
    void inotifyEvent();
    void checkEditDone();

  private:
    KUrl mUrl;
    QString mMimeType;
    bool mOpenWith;
    K3Process *mEditor;

    int mInotifyFd;
    int mInotifyWatch;
    bool mHaveInotify;

    bool mFileOpen;
    bool mEditorRunning;

    bool mFileModified;

    QTimer mTimer;
    QTime mEditTime;

    bool mError;
};

}

#endif
