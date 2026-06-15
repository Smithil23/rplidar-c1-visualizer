#include "ExportDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>

ExportDialog::ExportDialog(int frameCount, QWidget* parent)
    : QDialog(parent)
    , m_numFrames(frameCount)
{
    setWindowTitle("Export scan data");
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout(this);
    auto* form   = new QFormLayout;

    // Format selector
    m_formatCombo = new QComboBox;
    m_formatCombo->addItem("CSV  (Excel / MATLAB)",    QVariant::fromValue(int(DataExporter::Format::CSV)));
    m_formatCombo->addItem("PCD  (PCL / ROS / CloudCompare)", QVariant::fromValue(int(DataExporter::Format::PCD)));
    m_formatCombo->addItem("PLY  (MeshLab / Blender)", QVariant::fromValue(int(DataExporter::Format::PLY)));
    m_formatCombo->addItem("JSON (Web / REST)",         QVariant::fromValue(int(DataExporter::Format::JSON)));
    m_formatCombo->addItem("MCAP (ROS bag compatible)", QVariant::fromValue(int(DataExporter::Format::ROSBag)));
    form->addRow("Format:", m_formatCombo);

    // Frame count info
    m_frameCount = new QLabel(QString("<b>%1</b> scan frame(s) will be exported").arg(frameCount));
    form->addRow("Data:", m_frameCount);

    // File path
    auto* pathRow = new QHBoxLayout;
    m_pathLabel   = new QLabel("(no path selected)");
    m_pathLabel->setWordWrap(true);
    m_browseBtn   = new QPushButton("Browse…");
    pathRow->addWidget(m_pathLabel, 1);
    pathRow->addWidget(m_browseBtn);
    form->addRow("Save to:", pathRow);

    layout->addLayout(form);

    // Buttons
    m_exportBtn = new QPushButton("Export");
    m_exportBtn->setDefault(true);
    m_exportBtn->setEnabled(false);
    auto* cancelBtn = new QPushButton("Cancel");
    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(m_exportBtn);
    layout->addLayout(btnRow);

    // Suggest a default path
    onFormatChanged(0);

    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExportDialog::onFormatChanged);
    connect(m_browseBtn, &QPushButton::clicked, this, &ExportDialog::onBrowse);
    connect(m_exportBtn, &QPushButton::clicked, this, &ExportDialog::onExport);
    connect(cancelBtn,   &QPushButton::clicked, this, &QDialog::reject);
}

DataExporter::Format ExportDialog::selectedFormat() const {
    return static_cast<DataExporter::Format>(m_formatCombo->currentData().toInt());
}

QString ExportDialog::selectedPath() const {
    return m_path;
}

void ExportDialog::onFormatChanged(int) {
    auto fmt   = selectedFormat();
    QString suggested = DataExporter::suggestFilename(fmt);
    m_path = QDir::homePath() + "/" + suggested;
    m_pathLabel->setText(m_path);
    m_exportBtn->setEnabled(!m_path.isEmpty());
}

void ExportDialog::onBrowse() {
    auto fmt = selectedFormat();
    QString ext = DataExporter::extension(fmt);
    QString filter = QString("%1 files (*.%2);;All files (*)").arg(ext.toUpper(), ext);
    QString chosen = QFileDialog::getSaveFileName(this, "Save scan data", m_path, filter);
    if (!chosen.isEmpty()) {
        m_path = chosen;
        m_pathLabel->setText(m_path);
        m_exportBtn->setEnabled(true);
    }
}

void ExportDialog::onExport() {
    accept();   // caller does the actual writing
}
