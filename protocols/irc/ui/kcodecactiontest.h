#ifndef KSELECTACTION_TEST_H
#define KSELECTACTION_TEST_H

#include <kmainwindow.h>

class KCodecAction;

class CodecActionTest : public KMainWindow
{
    Q_OBJECT

public:
    CodecActionTest(QWidget* parent = 0);

public Q_SLOTS:
    void triggered(QAction* action);
    void triggered(int index);
    void triggered(const QString& text);
    void triggered(QTextCodec *codec);

    void slotActionTriggered(bool state);

    void addAction();
    void removeAction();

private:
    KCodecAction* m_comboCodec;
    KCodecAction* m_buttonCodec;
};

#endif
