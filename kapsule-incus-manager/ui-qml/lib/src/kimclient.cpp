#include "kimclient.h"
#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace KIM {

static constexpr auto SVC  = "org.KapsuleIncusManager";
static constexpr auto PATH = "/org/KapsuleIncusManager";
static constexpr auto IFC  = "org.KapsuleIncusManager";

static QVariantList parseArray(const QString &json)
{
    const auto doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) return {};
    QVariantList out;
    for (const auto &v : doc.array()) out.append(v.toVariant());
    return out;
}

static QString toJson(const QVariantMap &m)
{
    return QJsonDocument(QJsonObject::fromVariantMap(m)).toJson(QJsonDocument::Compact);
}

// Wire a pending call: on finish emit signal(QVariantList)
#define WATCH_LIST(call, sig) \
    { auto *w = new QDBusPendingCallWatcher((call), this); \
      connect(w, &QDBusPendingCallWatcher::finished, this, \
        [this,w](QDBusPendingCallWatcher*){ \
          QDBusPendingReply<QString> r = *w; w->deleteLater(); \
          if (r.isError()) { emit error(r.error().message()); return; } \
          emit sig(parseArray(r.value())); }); }

// Wire a pending call: on finish just report errors
#define WATCH_OP(call) \
    { auto *w = new QDBusPendingCallWatcher((call), this); \
      connect(w, &QDBusPendingCallWatcher::finished, this, \
        [this,w](QDBusPendingCallWatcher*){ \
          QDBusPendingReply<QString> r = *w; w->deleteLater(); \
          if (r.isError()) emit error(r.error().message()); }); }

// Wire a pending call that returns a plain string: emit sig(name, string)
#define WATCH_STR(call, name, sig) \
    { auto *w = new QDBusPendingCallWatcher((call), this); \
      connect(w, &QDBusPendingCallWatcher::finished, this, \
        [this,w,name](QDBusPendingCallWatcher*){ \
          QDBusPendingReply<QString> r = *w; w->deleteLater(); \
          if (r.isError()) { emit error(r.error().message()); return; } \
          emit sig(name, r.value()); }); }

KimClient::KimClient(QObject *parent) : QObject(parent) { connectToDaemon(); }
KimClient::~KimClient() = default;
bool KimClient::isConnected() const { return m_connected; }

void KimClient::connectToDaemon()
{
    m_iface = new QDBusInterface(SVC, PATH, IFC, QDBusConnection::sessionBus(), this);
    m_connected = m_iface->isValid();
    emit connectedChanged(m_connected);
    if (!m_connected) { emit error("Cannot connect to KIM daemon"); return; }

    auto bus = QDBusConnection::sessionBus();
    bus.connect(SVC, PATH, IFC, "EventReceived",
                this, SLOT(_onEventReceived(QString,QString,QString,QString)));
    bus.connect(SVC, PATH, IFC, "OperationCompleted",
                this, SLOT(_onOperationCompleted(QString,QString,QString)));
    bus.connect(SVC, PATH, IFC, "InstanceStateChanged",
                this, SLOT(_onInstanceStateChanged(QString,QString,QString)));
    bus.connect(SVC, PATH, IFC, "ResourceUsageUpdated",
                this, SLOT(_onResourceUsageUpdated(QString,QString,double,qulonglong,qulonglong)));
}

void KimClient::_onEventReceived(const QString &type, const QString &project,
                                  const QString &ts, const QString &payload)
{
    QVariantMap e;
    e["type"] = type; e["project"] = project; e["timestamp"] = ts;
    e["metadata"] = QJsonDocument::fromJson(payload.toUtf8()).object().toVariantMap();
    emit eventReceived(e);
}

void KimClient::_onOperationCompleted(const QString &, const QString &, const QString &)
{ listOperations(); }

void KimClient::_onInstanceStateChanged(const QString &name, const QString &,
                                         const QString &status)
{ emit instanceStateChanged(name, status); }

void KimClient::_onResourceUsageUpdated(const QString &name, const QString &project,
                                         double cpu, qulonglong mem, qulonglong disk)
{
    QVariantMap u;
    u["name"]=name; u["project"]=project;
    u["cpu_usage"]=cpu; u["memory_usage_bytes"]=mem; u["disk_usage_bytes"]=disk;
    emit resourceUsageUpdated(u);
}

// ── Instances ─────────────────────────────────────────────────────────────────
void KimClient::listInstances(const QString &p, const QString &r)
{ WATCH_LIST(m_iface->asyncCall("ListInstances", p, r, QString()), instancesListed) }

void KimClient::createInstance(const QVariantMap &c)
{ WATCH_OP(m_iface->asyncCall("CreateInstance", toJson(c))) }

void KimClient::startInstance(const QString &n, const QString &p)
{ WATCH_OP(m_iface->asyncCall("ChangeInstanceState", n, p, QString("start"), false, 30)) }

void KimClient::stopInstance(const QString &n, bool f, const QString &p)
{ WATCH_OP(m_iface->asyncCall("ChangeInstanceState", n, p, QString("stop"), f, 30)) }

void KimClient::restartInstance(const QString &n, bool f, const QString &p)
{ WATCH_OP(m_iface->asyncCall("ChangeInstanceState", n, p, QString("restart"), f, 30)) }

void KimClient::freezeInstance(const QString &n, const QString &p)
{ WATCH_OP(m_iface->asyncCall("ChangeInstanceState", n, p, QString("freeze"), false, 30)) }

void KimClient::deleteInstance(const QString &n, bool f, const QString &p)
{ WATCH_OP(m_iface->asyncCall("DeleteInstance", n, p, f)) }

void KimClient::renameInstance(const QString &n, const QString &nn, const QString &p)
{ WATCH_OP(m_iface->asyncCall("RenameInstance", n, nn, p)) }

// ── Networks ──────────────────────────────────────────────────────────────────
void KimClient::listNetworks(const QString &p)
{ WATCH_LIST(m_iface->asyncCall("ListNetworks", p, QString()), networksListed) }

void KimClient::createNetwork(const QVariantMap &c)
{ WATCH_OP(m_iface->asyncCall("CreateNetwork", toJson(c))) }

void KimClient::deleteNetwork(const QString &n)
{ WATCH_OP(m_iface->asyncCall("DeleteNetwork", n, QString())) }

// ── Storage ───────────────────────────────────────────────────────────────────
void KimClient::listStoragePools()
{ WATCH_LIST(m_iface->asyncCall("ListStoragePools", QString()), storagePoolsListed) }

void KimClient::createStoragePool(const QVariantMap &c)
{ WATCH_OP(m_iface->asyncCall("CreateStoragePool", toJson(c))) }

void KimClient::deleteStoragePool(const QString &n)
{ WATCH_OP(m_iface->asyncCall("DeleteStoragePool", n)) }

// ── Images ────────────────────────────────────────────────────────────────────
void KimClient::listImages(const QString &r)
{ WATCH_LIST(m_iface->asyncCall("ListImages", r), imagesListed) }

void KimClient::pullImage(const QString &r, const QString &image, const QString &alias)
{ WATCH_OP(m_iface->asyncCall("PullImage", r, image, alias)) }

void KimClient::deleteImage(const QString &fp)
{ WATCH_OP(m_iface->asyncCall("DeleteImage", fp)) }

// ── Profiles ──────────────────────────────────────────────────────────────────
void KimClient::listProfiles(const QString &p)
{ WATCH_LIST(m_iface->asyncCall("ListProfiles", p, QString()), profilesListed) }

void KimClient::listProfilePresets()
{ WATCH_LIST(m_iface->asyncCall("ListProfilePresets"), profilePresetsListed) }

void KimClient::createProfile(const QVariantMap &c)
{ WATCH_OP(m_iface->asyncCall("CreateProfile", toJson(c))) }

void KimClient::deleteProfile(const QString &n)
{ WATCH_OP(m_iface->asyncCall("DeleteProfile", n, QString())) }

// ── Projects ──────────────────────────────────────────────────────────────────
void KimClient::listProjects()
{ WATCH_LIST(m_iface->asyncCall("ListProjects", QString()), projectsListed) }

void KimClient::createProject(const QVariantMap &c)
{ WATCH_OP(m_iface->asyncCall("CreateProject", toJson(c))) }

void KimClient::deleteProject(const QString &n)
{ WATCH_OP(m_iface->asyncCall("DeleteProject", n)) }

// ── Cluster ───────────────────────────────────────────────────────────────────
void KimClient::listClusterMembers()
{ WATCH_LIST(m_iface->asyncCall("ListClusterMembers", QString()), clusterMembersListed) }

void KimClient::evacuateClusterMember(const QString &n)
{ WATCH_OP(m_iface->asyncCall("EvacuateClusterMember", n)) }

void KimClient::restoreClusterMember(const QString &n)
{ WATCH_OP(m_iface->asyncCall("RestoreClusterMember", n)) }

void KimClient::removeClusterMember(const QString &n)
{ WATCH_OP(m_iface->asyncCall("RemoveClusterMember", n)) }

// ── Remotes ───────────────────────────────────────────────────────────────────
void KimClient::listRemotes()
{ WATCH_LIST(m_iface->asyncCall("ListRemotes"), remotesListed) }

void KimClient::addRemote(const QVariantMap &c)
{ WATCH_OP(m_iface->asyncCall("AddRemote", toJson(c))) }

void KimClient::removeRemote(const QString &n)
{ WATCH_OP(m_iface->asyncCall("RemoveRemote", n)) }

void KimClient::activateRemote(const QString &n)
{ WATCH_OP(m_iface->asyncCall("ActivateRemote", n)) }

// ── Operations ────────────────────────────────────────────────────────────────
void KimClient::listOperations()
{ WATCH_LIST(m_iface->asyncCall("ListOperations", QString()), operationsListed) }

void KimClient::cancelOperation(const QString &id)
{ WATCH_OP(m_iface->asyncCall("CancelOperation", id)) }

// ── Console / Exec ────────────────────────────────────────────────────────────
void KimClient::consoleInstance(const QString &name, const QString &project,
                                 const QString &type, int width, int height)
{ WATCH_STR(m_iface->asyncCall("ConsoleInstance", name, project, type, width, height),
            name, consoleUrlReady) }

void KimClient::execInstance(const QString &name, const QString &project,
                              const QString &command, int width, int height)
{ WATCH_STR(m_iface->asyncCall("ExecInstance", name, project, command, width, height),
            name, execUrlReady) }

} // namespace KIM
