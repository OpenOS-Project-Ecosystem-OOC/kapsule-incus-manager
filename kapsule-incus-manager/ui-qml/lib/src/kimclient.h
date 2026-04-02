#pragma once

#include "types.h"
#include <QObject>
#include <QDBusInterface>
#include <QVariantMap>
#include <QVariantList>

namespace KIM {

// D-Bus client for the KIM daemon.
// Exposes the full daemon API as Qt signals and slots.
// License: LGPL-2.1-or-later
class KimClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
    explicit KimClient(QObject *parent = nullptr);
    ~KimClient() override;

    bool isConnected() const;

    Q_INVOKABLE void listInstances(const QString &project = {}, const QString &remote = {});
    Q_INVOKABLE void createInstance(const QVariantMap &config);
    Q_INVOKABLE void startInstance(const QString &name, const QString &project = {});
    Q_INVOKABLE void stopInstance(const QString &name, bool force = false, const QString &project = {});
    Q_INVOKABLE void restartInstance(const QString &name, bool force = false, const QString &project = {});
    Q_INVOKABLE void freezeInstance(const QString &name, const QString &project = {});
    Q_INVOKABLE void deleteInstance(const QString &name, bool force = false, const QString &project = {});
    Q_INVOKABLE void renameInstance(const QString &name, const QString &newName, const QString &project = {});

    Q_INVOKABLE void listNetworks(const QString &project = {});
    Q_INVOKABLE void createNetwork(const QVariantMap &config);
    Q_INVOKABLE void deleteNetwork(const QString &name);

    Q_INVOKABLE void listStoragePools();
    Q_INVOKABLE void createStoragePool(const QVariantMap &config);
    Q_INVOKABLE void deleteStoragePool(const QString &name);

    Q_INVOKABLE void listImages(const QString &remote = {});
    Q_INVOKABLE void pullImage(const QString &remote, const QString &image,
                               const QString &alias = {});
    Q_INVOKABLE void deleteImage(const QString &fingerprint);

    Q_INVOKABLE void listProfiles(const QString &project = {});
    Q_INVOKABLE void listProfilePresets();
    Q_INVOKABLE void createProfile(const QVariantMap &config);
    Q_INVOKABLE void deleteProfile(const QString &name);

    Q_INVOKABLE void listProjects();
    Q_INVOKABLE void createProject(const QVariantMap &config);
    Q_INVOKABLE void deleteProject(const QString &name);

    Q_INVOKABLE void listClusterMembers();
    Q_INVOKABLE void evacuateClusterMember(const QString &name);
    Q_INVOKABLE void restoreClusterMember(const QString &name);
    Q_INVOKABLE void removeClusterMember(const QString &name);

    Q_INVOKABLE void listRemotes();
    Q_INVOKABLE void addRemote(const QVariantMap &config);
    Q_INVOKABLE void removeRemote(const QString &name);
    Q_INVOKABLE void activateRemote(const QString &name);

    Q_INVOKABLE void listOperations();
    Q_INVOKABLE void cancelOperation(const QString &id);

    // Returns a ws:// URL string via the consoleUrlReady signal
    Q_INVOKABLE void consoleInstance(const QString &name, const QString &project = {},
                                     const QString &type = QStringLiteral("console"),
                                     int width = 80, int height = 24);
    // Returns a ws:// URL string via the execUrlReady signal
    Q_INVOKABLE void execInstance(const QString &name, const QString &project = {},
                                  const QString &command = QStringLiteral("/bin/bash"),
                                  int width = 80, int height = 24);

signals:
    void connectedChanged(bool connected);
    void instancesListed(const QVariantList &instances);
    void networksListed(const QVariantList &networks);
    void storagePoolsListed(const QVariantList &pools);
    void imagesListed(const QVariantList &images);
    void profilesListed(const QVariantList &profiles);
    void profilePresetsListed(const QVariantList &presets);
    void projectsListed(const QVariantList &projects);
    void clusterMembersListed(const QVariantList &members);
    void remotesListed(const QVariantList &remotes);
    void operationsListed(const QVariantList &operations);
    void instanceStateChanged(const QString &name, const QString &status);
    void resourceUsageUpdated(const QVariantMap &usage);
    void eventReceived(const QVariantMap &event);
    void consoleUrlReady(const QString &name, const QString &wsUrl);
    void execUrlReady(const QString &name, const QString &wsUrl);
    void error(const QString &message);

private slots:
    void _onEventReceived(const QString &type, const QString &project,
                          const QString &timestamp, const QString &payload);
    void _onOperationCompleted(const QString &opId, const QString &status,
                               const QString &opJson);
    void _onInstanceStateChanged(const QString &name, const QString &project,
                                 const QString &status);
    void _onResourceUsageUpdated(const QString &name, const QString &project,
                                 double cpuUsage, qulonglong memBytes,
                                 qulonglong diskBytes);

private:
    QDBusInterface *m_iface    = nullptr;
    bool            m_connected = false;
    void connectToDaemon();
};

} // namespace KIM
