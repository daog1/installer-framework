/**************************************************************************
**
** This file is part of Qt SDK**
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).*
**
** Contact:  Nokia Corporation qt-info@nokia.com**
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception version
** 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you are unsure which license is appropriate for your use, please contact
** (qt-info@nokia.com).
**
**************************************************************************/
#include "settings.h"

#include "common/errors.h"
#include "common/repository.h"
#include "qinstallerglobal.h"

#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

#include <QtXml/QDomDocument>

using namespace QInstaller;


// -- Settings::Private

class Settings::Private : public QSharedData
{
public:
    QString prefix;
    QString logo;
    QString logoSmall;
    QString title;
    QString maintenanceTitle;
    QString name;
    QString version;
    QString publisher;
    QString url;
    QString watermark;
    QString background;
    QString runProgram;
    QString runProgramDescription;
    QString startMenuDir;
    QString targetDir;
    QString adminTargetDir;
    QString icon;
    QString removeTargetDir;
    QString uninstallerName;
    QString uninstallerIniFile;
    QString configurationFileName;
    QSet<Repository> m_repositories;
    QSet<Repository> m_userRepositories;
    QStringList certificateFiles;
    QByteArray privateKey;
    QByteArray publicKey;

    QString makeAbsolutePath(const QString &p) const
    {
        if (QFileInfo(p).isAbsolute())
            return p;
        return prefix + QLatin1String("/") + p;
    }
};


static QString readChild(const QDomElement &el, const QString &child, const QString &defValue = QString())
{
    const QDomNodeList list = el.elementsByTagName(child);
    if (list.size() > 1)
        throw Error(QObject::tr("Multiple %1 elements found, but only one allowed.").arg(child));
    return list.isEmpty() ? defValue : list.at(0).toElement().text();
}

#if 0
static QStringList readChildList(const QDomElement &el, const QString &child)
{
    const QDomNodeList list = el.elementsByTagName(child);
    QStringList res;
    for (int i = 0; i < list.size(); ++i)
        res += list.at(i).toElement().text();
    return res;
}
#endif

static QString splitTrimmed(const QString &string)
{
    if (string.isEmpty())
        return QString();

    const QStringList input = string.split(QRegExp(QLatin1String("\n|\r\n")));

    QStringList result;
    foreach (const QString &line, input)
        result.append(line.trimmed());
    result.append(QString());

    return result.join(QLatin1String("\n"));
}


// -- InstallerSettings

Settings::Settings()
    : d(new Private)
{
}

Settings::~Settings()
{
}

Settings::Settings(const Settings &other)
    : d(other.d)
{
}

Settings& Settings::operator=(const Settings &other)
{
    Settings copy(other);
    std::swap(d, copy.d);
    return *this;
}

/*!
    @throws QInstallerError
*/
Settings Settings::fromFileAndPrefix(const QString &path, const QString &prefix)
{
    QDomDocument doc;
    QFile file(path);
    QFile overrideConfig(QLatin1String(":/overrideconfig.xml"));

    if (overrideConfig.exists())
        file.setFileName(overrideConfig.fileName());

    if (!file.open(QIODevice::ReadOnly))
        throw Error(tr("Could not open settings file %1 for reading: %2").arg(path, file.errorString()));

    QString msg;
    int line = 0;
    int col = 0;
    if (!doc.setContent(&file, &msg, &line, &col)) {
        throw Error(tr("Could not parse settings file %1: %2:%3: %4").arg(path, QString::number(line),
            QString::number(col), msg));
    }

    const QDomElement root = doc.documentElement();
    if (root.tagName() != QLatin1String("Installer"))
        throw Error(tr("%1 is not valid: Installer root node expected"));

    Settings s;
    s.d->prefix = prefix;
    s.d->logo = readChild(root, QLatin1String("Logo"));
    s.d->logoSmall = readChild(root, QLatin1String("LogoSmall"));
    s.d->title = readChild(root, QLatin1String("Title"));
    s.d->maintenanceTitle = readChild(root, QLatin1String("MaintenanceTitle"));
    s.d->name = readChild(root, scName);
    s.d->version = readChild(root, scVersion);
    s.d->publisher = readChild(root, QLatin1String("Publisher"));
    s.d->url = readChild(root, QLatin1String("ProductUrl"));
    s.d->watermark = readChild(root, QLatin1String("Watermark"));
    s.d->background = readChild(root, QLatin1String("Background"));
    s.d->runProgram = readChild(root, QLatin1String("RunProgram"));
    s.d->runProgramDescription = readChild(root, QLatin1String("RunProgramDescription"));
    s.d->startMenuDir = readChild(root, QLatin1String("StartMenuDir"));
    s.d->targetDir = readChild(root, scTargetDir);
    s.d->adminTargetDir = readChild(root, QLatin1String("AdminTargetDir"));
    s.d->icon = readChild(root, QLatin1String("Icon"));
    s.d->removeTargetDir = readChild(root, QLatin1String("RemoveTargetDir"), scTrue);
    s.d->uninstallerName = readChild(root, QLatin1String("UninstallerName"), QLatin1String("uninstall"));
    s.d->uninstallerIniFile = readChild(root, QLatin1String("UninstallerIniFile"), s.d->uninstallerName
        + QLatin1String(".ini"));
    s.d->privateKey = splitTrimmed(readChild(root, QLatin1String("PrivateKey"))).toLatin1();
    s.d->publicKey = splitTrimmed(readChild(root, QLatin1String("PublicKey"))).toLatin1();
    s.d->configurationFileName = readChild(root, QLatin1String("TargetConfigurationFile"),
        QLatin1String("components.xml"));

    const QDomNodeList reposTags = root.elementsByTagName(QLatin1String("RemoteRepositories"));
    for (int a = 0; a < reposTags.count(); ++a) {
        const QDomNodeList repos = reposTags.at(a).toElement().elementsByTagName(QLatin1String("Repository"));
        for (int i = 0; i < repos.size(); ++i) {
            Repository r;
            const QDomNodeList children = repos.at(i).childNodes();
            for (int j = 0; j < children.size(); ++j) {
                if (children.at(j).toElement().tagName() == QLatin1String("Url"))
                    r.setUrl(children.at(j).toElement().text());
            }
            s.d->m_repositories.insert(r);
        }
    }

    const QDomNodeList certs = root.elementsByTagName(QLatin1String("SigningCertificate"));
    for (int i=0; i < certs.size(); ++i) {
        const QString str = certs.at(i).toElement().text();
        if (str.isEmpty())
            continue;
        s.d->certificateFiles.push_back(s.d->makeAbsolutePath(str));
    }
    return s;
}

QString Settings::maintenanceTitle() const
{
    return d->maintenanceTitle;
}

QString Settings::logo() const
{
    return d->makeAbsolutePath(d->logo);
}

QString Settings::logoSmall() const
{
    return d->makeAbsolutePath(d->logoSmall);
}

QString Settings::title() const
{
    return d->title;
}

QString Settings::applicationName() const
{
    return d->name;
}

QString Settings::applicationVersion() const
{
    return d->version;
}

QString Settings::publisher() const
{
    return d->publisher;
}

QString Settings::url() const
{
    return d->url;
}

QString Settings::watermark() const
{
    return d->makeAbsolutePath(d->watermark);
}

QString Settings::background() const
{
    return d->makeAbsolutePath(d->background);
}

QString Settings::icon() const
{
#if defined(Q_WS_MAC)
    return d->makeAbsolutePath(d->icon) + QLatin1String(".icns");
#elif defined(Q_WS_WIN)
    return d->makeAbsolutePath(d->icon) + QLatin1String(".ico");
#endif
    return d->makeAbsolutePath(d->icon) + QLatin1String(".png");
}

QString Settings::removeTargetDir() const
{
    return d->removeTargetDir;
}

QString Settings::uninstallerName() const
{
    if (d->uninstallerName.isEmpty())
        return QLatin1String("uninstall");
    return d->uninstallerName;
}

QString Settings::uninstallerIniFile() const
{
    return d->uninstallerIniFile;
}

QString Settings::runProgram() const
{
    return d->runProgram;
}

QString Settings::runProgramDescription() const
{
    return d->runProgramDescription;
}

QString Settings::startMenuDir() const
{
    return d->startMenuDir;
}

QString Settings::targetDir() const
{
    return d->targetDir;
}

QString Settings::adminTargetDir() const
{
    return d->adminTargetDir;
}

QString Settings::configurationFileName() const
{
    return d->configurationFileName;
}

QStringList Settings::certificateFiles() const
{
    return d->certificateFiles;
}

QByteArray Settings::privateKey() const
{
    return d->privateKey;
}

QByteArray Settings::publicKey() const
{
    return d->publicKey;
}

QList<Repository> Settings::repositories() const
{
    return (d->m_repositories + d->m_userRepositories).toList();
}

void Settings::setTemporaryRepositories(const QList<Repository> &repos, bool replace)
{
    if (replace)
        d->m_repositories = repos.toSet();
    else
        d->m_repositories.unite(repos.toSet());
}

QList<Repository> Settings::userRepositories() const
{
    return d->m_userRepositories.toList();
}

void Settings::addUserRepositories(const QList<Repository> &repositories)
{
    d->m_userRepositories.unite(repositories.toSet());
}