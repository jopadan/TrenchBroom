/*
 Copyright (C) 2010 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "GameDialog.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>

#include "PreferenceManager.h"
#include "TrenchBroomApp.h"
#include "mdl/GameFactory.h"
#include "ui/BorderLine.h"
#include "ui/GameListBox.h"
#include "ui/QtUtils.h"
#include "ui/ViewConstants.h"

#include <cassert>
#include <string>

Q_DECLARE_METATYPE(tb::mdl::MapFormat)

namespace tb::ui
{

std::optional<std::tuple<std::string, mdl::MapFormat>> GameDialog::showNewDocumentDialog(
  QWidget* parent)
{
  auto dialog = GameDialog{
    "Select Game",
    R"(Select a game from the list on the right, then click OK. Once the new document is created, you can set up mod directories, entity definitions and materials by going to the map inspector, the entity inspector and the face inspector, respectively.)",
    GameDialog::DialogType::New,
    parent};

  if (dialog.exec() == QDialog::Rejected)
  {
    return std::nullopt;
  }

  return std::tuple{
    dialog.currentGameName(),
    dialog.currentMapFormat(),
  };
}

std::optional<std::tuple<std::string, mdl::MapFormat>> GameDialog::showOpenDocumentDialog(
  QWidget* parent)
{
  auto dialog = GameDialog{
    "Select Game",
    R"(TrenchBroom was unable to detect the game for the map document. Please choose a game in the game list and click OK.)",
    GameDialog::DialogType::Open,
    parent};

  if (dialog.exec() == QDialog::Rejected)
  {
    return std::nullopt;
  }

  return std::tuple{
    dialog.currentGameName(),
    dialog.currentMapFormat(),
  };
}

std::string GameDialog::currentGameName() const
{
  return m_gameListBox->selectedGameName();
}

static mdl::MapFormat formatFromUserData(const QVariant& variant)
{
  return variant.canConvert<mdl::MapFormat>() ? variant.value<mdl::MapFormat>()
                                              : mdl::MapFormat::Unknown;
}

static QVariant formatToUserData(const mdl::MapFormat format)
{
  return QVariant::fromValue(format);
}

mdl::MapFormat GameDialog::currentMapFormat() const
{
  const auto userData = m_mapFormatComboBox->currentData();
  assert(userData.isValid());

  return formatFromUserData(userData);
}

void GameDialog::currentGameChanged(const QString& gameName)
{
  updateMapFormats(gameName.toStdString());
  m_okButton->setEnabled(!gameName.isEmpty());
}

void GameDialog::gameSelected(const QString& /* gameName */)
{
  accept();
}

void GameDialog::openPreferencesClicked()
{
  auto& app = TrenchBroomApp::instance();
  app.openPreferences();
}

GameDialog::GameDialog(
  const QString& title, const QString& infoText, const DialogType type, QWidget* parent)
  : QDialog{parent}
  , m_dialogType{type}
{
  createGui(title, infoText);
  updateMapFormats("");
  connectObservers();
}

void GameDialog::createGui(const QString& title, const QString& infoText)
{
  setWindowTitle(title);
  setWindowIconTB(this);

  auto* infoPanel = createInfoPanel(title, infoText);
  auto* selectionPanel = createSelectionPanel();
  selectionPanel->setMinimumWidth(300);

  auto* innerLayout = new QHBoxLayout{};
  innerLayout->setContentsMargins(QMargins{});
  innerLayout->setSpacing(0);
  innerLayout->addWidget(infoPanel, 1);
  innerLayout->addWidget(new BorderLine{BorderLine::Direction::Vertical}, 1);
  innerLayout->addWidget(selectionPanel, 1);

  auto* buttonBox = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel};
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  m_okButton = buttonBox->button(QDialogButtonBox::Ok);
  m_okButton->setEnabled(false);

  auto* outerLayout = new QVBoxLayout{};
  outerLayout->setContentsMargins(QMargins{});
  outerLayout->setSpacing(0);
  outerLayout->addLayout(innerLayout, 1);
  outerLayout->addLayout(wrapDialogButtonBox(buttonBox), 1);
  insertTitleBarSeparator(outerLayout);

  setLayout(outerLayout);
}

QWidget* GameDialog::createInfoPanel(const QString& title, const QString& infoText)
{
  auto* infoPanel = new QWidget{};

  auto* header = new QLabel{title};
  makeHeader(header);

  auto* info = new QLabel{infoText};
  info->setWordWrap(true);

  auto* setupMsg = new QLabel{
    R"(To set up the game paths, click on the button below to open the preferences dialog.)"};
  setupMsg->setWordWrap(true);

  m_openPreferencesButton = new QPushButton{"Open preferences..."};
  m_openPreferencesButton->setToolTip(
    "Open the preferences dialog to manage game paths,");

  auto* layout = new QVBoxLayout{};
  layout->setSpacing(0);
  layout->setContentsMargins(20, 20, 20, 20);

  layout->addWidget(header);
  layout->addSpacing(20);
  layout->addWidget(info);
  layout->addSpacing(10);
  layout->addWidget(setupMsg);
  layout->addSpacing(10);
  layout->addWidget(m_openPreferencesButton, 0, Qt::AlignHCenter);
  infoPanel->setLayout(layout);
  infoPanel->setMaximumWidth(350);

  connect(
    m_openPreferencesButton,
    &QPushButton::clicked,
    this,
    &GameDialog::openPreferencesClicked);

  return infoPanel;
}

QWidget* GameDialog::createSelectionPanel()
{
  auto* panel = new QWidget{};

  m_gameListBox = new GameListBox{};
  m_gameListBox->setToolTip("Double click on a game to select it");

  auto* label = new QLabel{"Map Format"};
  makeEmphasized(label);

  m_mapFormatComboBox = new QComboBox{};
  m_mapFormatComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

  auto* mapFormatLayout = new QHBoxLayout{};
  mapFormatLayout->setContentsMargins(
    LayoutConstants::WideHMargin,
    LayoutConstants::NarrowVMargin,
    LayoutConstants::WideHMargin,
    LayoutConstants::NarrowVMargin);
  mapFormatLayout->setSpacing(LayoutConstants::WideHMargin);
  mapFormatLayout->addWidget(label, 0, Qt::AlignRight | Qt::AlignVCenter);
  mapFormatLayout->addWidget(m_mapFormatComboBox, 1, Qt::AlignLeft | Qt::AlignVCenter);

  auto* outerSizer = new QVBoxLayout{};
  outerSizer->setContentsMargins(QMargins{});
  outerSizer->setSpacing(0);
  outerSizer->addWidget(m_gameListBox, 1);
  outerSizer->addWidget(new BorderLine{}, 1);
  outerSizer->addLayout(mapFormatLayout);
  panel->setLayout(outerSizer);

  connect(
    m_gameListBox,
    &GameListBox::currentGameChanged,
    this,
    &GameDialog::currentGameChanged);
  connect(
    m_gameListBox, &GameListBox::selectCurrentGame, this, &GameDialog::gameSelected);

  return panel;
}

void GameDialog::updateMapFormats(const std::string& gameName)
{
  const auto& gameFactory = mdl::GameFactory::instance();
  const auto fileFormats =
    gameName.empty() ? std::vector<std::string>{} : gameFactory.fileFormats(gameName);

  m_mapFormatComboBox->clear();
  if (m_dialogType == DialogType::Open)
  {
    m_mapFormatComboBox->addItem(
      tr("Autodetect"), formatToUserData(mdl::MapFormat::Unknown));
  }

  for (const auto& fileFormat : fileFormats)
  {
    const auto mapFormat = mdl::formatFromName(fileFormat);
    m_mapFormatComboBox->addItem(
      QString::fromStdString(fileFormat), formatToUserData(mapFormat));
  }

  m_mapFormatComboBox->setEnabled(m_mapFormatComboBox->count() > 1);
  if (m_mapFormatComboBox->count() > 0)
  {
    m_mapFormatComboBox->setCurrentIndex(0);
  }
}

void GameDialog::connectObservers()
{
  auto& prefs = PreferenceManager::instance();
  m_notifierConnection +=
    prefs.preferenceDidChangeNotifier.connect(this, &GameDialog::preferenceDidChange);
}

void GameDialog::preferenceDidChange(const std::filesystem::path&)
{
  m_gameListBox->reloadGameInfos();
  m_okButton->setEnabled(!currentGameName().empty());
}

} // namespace tb::ui
