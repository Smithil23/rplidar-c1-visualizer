#pragma once
#include "core/DataExporter.h"
#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

class ExportDialog : public QDialog {
    Q_OBJECT
public:
    explicit ExportDialog(int frameCount, QWidget* parent = nullptr);
    DataExporter::Format selectedFormat() const;
    QString              selectedPath()   const;

private slots:
    void onFormatChanged(int index);
    void onBrowse();
    void onExport();

private:
    QComboBox*   m_formatCombo;
    QLabel*      m_pathLabel;
    QPushButton* m_browseBtn;
    QPushButton* m_exportBtn;
    QLabel*      m_frameCount;
    QString      m_path;
    int          m_numFrames;
};
