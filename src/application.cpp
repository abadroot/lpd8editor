#include "application.h"

#include "db.h"
#include "midiio.h"
#include "programsmodel.h"
#include "utils.h"

#include <QFileInfo>
#include <QSettings>
#include <QSqlTableModel>
#include <QtDebug>

#include <exception>

static const QString SETTINGS_KEY_ACTIVE_PROGRAM_ID = "active/program_id";

Application::Application(QObject *parent):
    QObject(parent),
    m_my_programs(Q_NULLPTR),
    m_programs(Q_NULLPTR),
    m_pads(Q_NULLPTR),
    m_knobs(Q_NULLPTR),
    m_midi_io(Q_NULLPTR)
{
    if (!initFilesystem()) {
        throw std::runtime_error("Failed filesystem initialization");
    }
    if (initDb(defaultDbPath()).isValid()) {
        throw std::runtime_error("Failed database initialization");
    }

    m_my_programs = new ProgramsModel(this);

    m_programs = new QSqlTableModel(this);
    m_programs->setTable("programs");
    m_programs->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_programs->select();

    m_pads = new QSqlTableModel(this);
    m_pads->setTable("pads");
    m_pads->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_pads->select();

    m_knobs = new QSqlTableModel(this);
    m_knobs->setTable("knobs");
    m_knobs->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_knobs->select();

    connect(this,
            &Application::activeProgramIdChanged,
            &Application::refreshModels);

    refreshModels();

    m_midi_io = new OldMidiIO(this);
}

QAbstractItemModel* Application::programs() const {
    return m_my_programs;
}

ProgramsModel* Application::myPrograms() const {
    return m_my_programs;
}

QAbstractItemModel* Application::pads() const {
    return m_pads;
}

QAbstractItemModel* Application::knobs() const {
    return m_knobs;
}

int Application::activeProgramId() const {
    const int programId = QSettings().value(SETTINGS_KEY_ACTIVE_PROGRAM_ID, -1).toInt();
    return isValidProgramId(programId) ? programId : -1;
}

OldMidiIO* Application::midiIO() const {
    Q_CHECK_PTR(m_midi_io);

    return m_midi_io;
}

int Application::activeProgramChannel() const
{
    Q_CHECK_PTR(programs());

    for (int row = 0 ; row < programs()->rowCount() ; ++row) {
        const int programId(getProgramId(programs(), row));
        if (programId == activeProgramId()) {
            return programs()->data(programs()->index(row, 2)).toInt();
        }
    }

    return -1;
}

void Application::setActiveProgramId(int programId) {
    if (QSettings().value(SETTINGS_KEY_ACTIVE_PROGRAM_ID).toInt() == programId) {
        return;
    }
    QSettings().setValue(SETTINGS_KEY_ACTIVE_PROGRAM_ID, programId);
    emit activeProgramIdChanged(programId);
    emit activeProgramChannelChanged(activeProgramChannel());
}

void Application::setActiveProgramChannel(int channel)
{
    Q_CHECK_PTR(programs());

    for (int row = 0 ; row < programs()->rowCount() ; ++row) {
        const int programId(getProgramId(programs(), row));
        if (programId == activeProgramId()) {
            const QModelIndex idx(programs()->index(row, 2));
            const int curChannel(programs()->data(idx).toInt());
            if (channel != curChannel) {
                programs()->setData(idx, channel);
                emit activeProgramChannelChanged(channel);
            }
        }
    }
}

void Application::refreshModels() {
    Q_CHECK_PTR(m_pads);
    Q_CHECK_PTR(m_knobs);

    const QString filter = QString("programId=%1").arg(activeProgramId());
    m_pads->setFilter(filter);
    m_knobs->setFilter(filter);
}

char getChar(const QSqlRecord& r, const QString& name) {
    return r.value(name).toChar().toLatin1();
}

void Application::sendProgram(int programId) {
    m_midi_io->sendProgram(program(programId));
}

pProgram Application::program(int programId) const {
    Q_CHECK_PTR(m_midi_io);

    pProgram p(new Program());
    p->id = static_cast<char>(programId);
    p->channel = 1;

    for (int i = 0 ; i < m_pads->rowCount() ; ++i ) {
        QSqlRecord r = m_pads->record(i);

        Q_ASSERT(r.contains("programId"));
        Q_ASSERT(r.contains("note"));
        Q_ASSERT(r.contains("pc"));
        Q_ASSERT(r.contains("cc"));
        Q_ASSERT(r.contains("toggle"));

        const int padIndex = r.value("controlId").toInt()-1;
        Pad& pad = p->pads[padIndex];
        pad.note = getChar(r, "note");
        pad.pc = getChar(r, "pc");
        pad.cc = getChar(r, "cc");
        pad.toggle = getChar(r, "toggle");
    }

    for (int i = 0 ; i < m_knobs->rowCount() ; ++i ) {
        QSqlRecord r = m_knobs->record(i);

        Q_ASSERT(r.contains("programId"));
        Q_ASSERT(r.contains("cc"));
        Q_ASSERT(r.contains("low"));
        Q_ASSERT(r.contains("high"));

        const int knobIndex = r.value("controlId").toInt()-1;
        Knob& knob = p->knobs[knobIndex];
        knob.cc = getChar(r, "cc");
        knob.low = getChar(r, "low");
        knob.high = getChar(r, "high");
    }

    return p;
}
