#include "Interpreter.h"

#include "Instruction.h"
#include "instructions.h"

#include <QStack>

Interpreter::Interpreter(QByteArray program) :
    tape(NULL),
    stdout_(new QTextStream(stdout)),
    stderr_(new QTextStream(stderr)),
    stdin_(new QTextStream(stdin)),
    timed_out_flag(false),
    m_program(program),
    m_max_cycles(-1)
{
    m_instructions.insert('>', new IncrementHeadInstruction(this));
    m_instructions.insert('<', new DecrementHeadInstruction(this));
    m_instructions.insert('+', new IncrementAtHeadInstruction(this));
    m_instructions.insert('-', new DecrementAtHeadInstruction(this));
    m_instructions.insert('.', new OutputHeadInstruction(this));
    m_instructions.insert(',', new InputHeadInstruction(this));
    m_instructions.insert('[', new BeginLoopInstruction(this));
    m_instructions.insert(']', new EndLoopInstruction(this));

    m_noop = new NoopInstruction(this);
}

Interpreter::~Interpreter()
{
    delete tape;

    QHashIterator<char, Instruction *> it(m_instructions);
    while (it.hasNext()) {
        it.next();
        delete it.value();
    }

    delete stdout_;
    delete stderr_;
    delete stdin_;

    delete m_noop;
}

void Interpreter::start()
{
    // parse for matching brackets
    QStack<int> stack;
    for (int i = 0; i < m_program.size(); i++) {
        if (m_program.at(i) == '[') {
            stack.push(i);
        } else if (m_program.at(i) == ']') {
            if (! stack.isEmpty()) {
                int start = stack.pop();
                matching_bracket.insert(i, start);
                matching_bracket.insert(start, i);
            } else {
                // just send control forward; ignore this close bracket
                matching_bracket.insert(i, i+1);
            }
        }
    }

    tape = new Tape;
    pc = 0;
    m_cycle_count = 0;
    while (pc < m_program.size()) {
        Instruction * instruction = m_instructions.value(m_program.at(pc), m_noop);
        instruction->execute();
        pc++;
        m_cycle_count++;
        if (m_max_cycles > 0 && m_cycle_count == m_max_cycles) {
            timed_out_flag = true;
            break;
        }
    }
}

void Interpreter::setCaptureOutput(bool on)
{
    delete this->stderr_;
    delete this->stdout_;

    if (on) {
        this->stderr_ = new QTextStream(&m_stderr_str);
        this->stdout_ = new QTextStream(&m_stdout_str);
    } else {
        this->stderr_ = new QTextStream(stderr);
        this->stdout_ = new QTextStream(stdout);
    }
}

void Interpreter::setInput(QByteArray input)
{
    m_input = input;
    delete stdin_;
    if (input.isNull())
        stdin_ = NULL;
    else
        stdin_ = new QTextStream(&m_input);
}

void Interpreter::setMaxCycles(qint64 cycles)
{
    m_max_cycles = cycles;
}

qint64 Interpreter::cycleCount() const
{
    return m_cycle_count;
}
