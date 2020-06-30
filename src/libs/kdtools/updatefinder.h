/****************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Installer Framework.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef UPDATEFINDER_H
#define UPDATEFINDER_H

#include "task.h"
#include "packagesource.h"
#include "filedownloader.h"
#include "updatesinfo_p.h"

#include <memory>
#include <QtCore/qset.h>
using QInstaller::PackageSource;
namespace KDUpdater {

class LocalPackageHub;
class Update;

class KDTOOLS_EXPORT UpdateFinder : public Task
{
    Q_OBJECT
    //class Private;

public:
    UpdateFinder();
    ~UpdateFinder();
    class Private
    {
    public:
        enum struct Resolution {
            AddPackage,
            KeepExisting,
            RemoveExisting
        };

        Private(UpdateFinder *qq)
            : q(qq)
            , downloadCompleteCount(0)
            , m_downloadsToComplete(0)
        {}

        ~Private()
        {
            clear();
        }

        struct Data {
            Data()
                : downloader(0) {}
            Data(const PackageSource &i, FileDownloader *d = 0)
                : info(i), downloader(d) {}

            PackageSource info;
            FileDownloader *downloader;
        };
        UpdateFinder *q;
        QHash<QString, Update *> updates;

        // Temporary structure that notes down information about updates.
        bool cancel;
        int downloadCompleteCount;
        int m_downloadsToComplete;
        QHash<UpdatesInfo *, Data> m_updatesInfoList;

        void clear();
        void computeUpdates();
        void cancelComputeUpdates();
        bool downloadUpdateXMLFiles();
        bool computeApplicableUpdates();

        QList<UpdateInfo> applicableUpdates(UpdatesInfo *updatesInfo);
        void createUpdateObjects(const PackageSource &source, const QList<UpdateInfo> &updateInfoList);
        Resolution checkPriorityAndVersion(const PackageSource &source, const QVariantHash &data) const;
        void slotDownloadDone();

        QSet<PackageSource> packageSources;
        std::weak_ptr<LocalPackageHub> m_localPackageHub;
    };


    QList<Update *> updates() const;

    void setLocalPackageHub(std::weak_ptr<LocalPackageHub> hub);
    void setPackageSources(const QSet<QInstaller::PackageSource> &sources);
    void addCompressedPackage(bool add) { m_compressedPackage = add; }
    bool isCompressedPackage() { return m_compressedPackage; }
private:
    void doRun();
    bool doStop();
    bool doPause();
    bool doResume();

private:
    bool m_compressedPackage;
    Private *d;
    Q_PRIVATE_SLOT(d, void slotDownloadDone())
};

} // namespace KDUpdater

#endif // UPDATEFINDER_H
