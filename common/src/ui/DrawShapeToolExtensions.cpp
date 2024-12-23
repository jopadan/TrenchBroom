/*
 Copyright (C) 2023 Kristian Duske

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

#include "DrawShapeToolExtensions.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QToolButton>

#include "mdl/BrushBuilder.h"
#include "mdl/WorldNode.h"
#include "ui/MapDocument.h"
#include "ui/QtUtils.h"

#include "kdl/memory_utils.h"

namespace tb::ui
{

DrawShapeToolCuboidExtension::DrawShapeToolCuboidExtension(
  std::weak_ptr<MapDocument> document)
  : DrawShapeToolExtension{std::move(document)}
{
}

const std::string& DrawShapeToolCuboidExtension::name() const
{
  static const auto name = std::string{"Cuboid"};
  return name;
}

const std::filesystem::path& DrawShapeToolCuboidExtension::iconPath() const
{
  static const auto path = std::filesystem::path{"ShapeTool_Cuboid.svg"};
  return path;
}

DrawShapeToolExtensionPage* DrawShapeToolCuboidExtension::createToolPage(QWidget* parent)
{
  return new DrawShapeToolExtensionPage{parent};
}

Result<std::vector<mdl::Brush>> DrawShapeToolCuboidExtension::createBrushes(
  const vm::bbox3d& bounds) const
{
  auto document = kdl::mem_lock(m_document);
  const auto game = document->game();
  const auto builder = mdl::BrushBuilder{
    document->world()->mapFormat(),
    document->worldBounds(),
    game->config().faceAttribsConfig.defaults};

  return builder.createCuboid(bounds, document->currentMaterialName())
    .transform([](auto brush) { return std::vector{std::move(brush)}; });
}

DrawShapeToolAxisAlignedShapeExtensionPage::DrawShapeToolAxisAlignedShapeExtensionPage(
  AxisAlignedShapeParameters& parameters, QWidget* parent)
  : DrawShapeToolExtensionPage{parent}
  , m_parameters{parameters}
{
  auto* axisLabel = new QLabel{tr("Axis: ")};
  auto* axisComboBox = new QComboBox{};
  axisComboBox->addItems({tr("X"), tr("Y"), tr("Z")});
  axisComboBox->setCurrentIndex(int(parameters.axis));

  connect(
    axisComboBox,
    QOverload<int>::of(&QComboBox::currentIndexChanged),
    this,
    [&](const auto index) { m_parameters.axis = vm::axis::type(index); });

  addWidget(axisLabel);
  addWidget(axisComboBox);
}

DrawShapeToolCircularShapeExtensionPage::DrawShapeToolCircularShapeExtensionPage(
  CircularShapeParameters& parameters, QWidget* parent)
  : DrawShapeToolAxisAlignedShapeExtensionPage{parameters, parent}
  , m_parameters{parameters}
{
  auto* numSidesLabel = new QLabel{tr("Number of Sides: ")};
  auto* numSidesBox = new QSpinBox{};
  numSidesBox->setRange(3, 256);
  numSidesBox->setValue(int(m_parameters.numSides));

  auto* radiusModeEdgeButton =
    createBitmapToggleButton("RadiusModeEdge.svg", tr("Radius is to edge"));
  radiusModeEdgeButton->setIconSize({24, 24});
  radiusModeEdgeButton->setObjectName("toolButton_withBorder");
  radiusModeEdgeButton->setChecked(m_parameters.radiusMode == mdl::RadiusMode::ToEdge);

  auto* radiusModeVertexButton =
    createBitmapToggleButton("RadiusModeVertex.svg", tr("Radius is to vertex"));
  radiusModeVertexButton->setIconSize({24, 24});
  radiusModeVertexButton->setObjectName("toolButton_withBorder");
  radiusModeVertexButton->setChecked(
    m_parameters.radiusMode == mdl::RadiusMode::ToVertex);

  auto* radiusModeButtonGroup = new QButtonGroup{};
  radiusModeButtonGroup->addButton(radiusModeEdgeButton);
  radiusModeButtonGroup->addButton(radiusModeVertexButton);

  connect(
    numSidesBox,
    QOverload<int>::of(&QSpinBox::valueChanged),
    this,
    [&](const auto numSides) { m_parameters.numSides = size_t(numSides); });
  connect(radiusModeEdgeButton, &QToolButton::clicked, this, [&]() {
    m_parameters.radiusMode = mdl::RadiusMode::ToEdge;
  });
  connect(radiusModeVertexButton, &QToolButton::clicked, this, [&]() {
    m_parameters.radiusMode = mdl::RadiusMode::ToVertex;
  });

  addWidget(numSidesLabel);
  addWidget(numSidesBox);
  addWidget(radiusModeEdgeButton);
  addWidget(radiusModeVertexButton);
}

DrawShapeToolCylinderShapeExtensionPage::DrawShapeToolCylinderShapeExtensionPage(
  std::weak_ptr<MapDocument> document,
  CylinderShapeParameters& parameters,
  QWidget* parent)
  : DrawShapeToolCircularShapeExtensionPage{parameters, parent}
  , m_parameters{parameters}
{
  auto* hollowCheckBox = new QCheckBox{tr("Hollow")};
  hollowCheckBox->setChecked(m_parameters.hollow);

  auto* thicknessLabel = new QLabel{tr("Thickness: ")};
  auto* thicknessBox = new QDoubleSpinBox{};
  thicknessBox->setEnabled(parameters.hollow);
  thicknessBox->setRange(1, 128);
  thicknessBox->setValue(parameters.thickness);

  connect(
    hollowCheckBox, &QCheckBox::toggled, this, [&, thicknessBox](const auto hollow) {
      m_parameters.hollow = hollow;
      thicknessBox->setEnabled(hollow);
    });
  connect(
    thicknessBox,
    QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    this,
    [&](const auto thickness) { m_parameters.thickness = thickness; });

  addWidget(hollowCheckBox);
  addWidget(thicknessLabel);
  addWidget(thicknessBox);
  addApplyButton(document);
}

DrawShapeToolCylinderExtension::DrawShapeToolCylinderExtension(
  std::weak_ptr<MapDocument> document)
  : DrawShapeToolExtension{std::move(document)}
  , m_parameters{vm::axis::z, 8, mdl::RadiusMode::ToEdge, false, 16.0}
{
}

const std::string& DrawShapeToolCylinderExtension::name() const
{
  static const auto name = std::string{"Cylinder"};
  return name;
}

const std::filesystem::path& DrawShapeToolCylinderExtension::iconPath() const
{
  static const auto path = std::filesystem::path{"ShapeTool_Cylinder.svg"};
  return path;
}

DrawShapeToolExtensionPage* DrawShapeToolCylinderExtension::createToolPage(
  QWidget* parent)
{
  return new DrawShapeToolCylinderShapeExtensionPage{m_document, m_parameters, parent};
}

Result<std::vector<mdl::Brush>> DrawShapeToolCylinderExtension::createBrushes(
  const vm::bbox3d& bounds) const
{
  auto document = kdl::mem_lock(m_document);
  const auto game = document->game();
  const auto builder = mdl::BrushBuilder{
    document->world()->mapFormat(),
    document->worldBounds(),
    game->config().faceAttribsConfig.defaults};
  return m_parameters.hollow
           ? builder.createHollowCylinder(
               bounds,
               m_parameters.thickness,
               m_parameters.numSides,
               m_parameters.radiusMode,
               m_parameters.axis,
               document->currentMaterialName())
           : builder
               .createCylinder(
                 bounds,
                 m_parameters.numSides,
                 m_parameters.radiusMode,
                 m_parameters.axis,
                 document->currentMaterialName())
               .transform([](auto brush) { return std::vector{std::move(brush)}; });
}

DrawShapeToolConeShapeExtensionPage::DrawShapeToolConeShapeExtensionPage(
  std::weak_ptr<MapDocument> document,
  CircularShapeParameters& parameters,
  QWidget* parent)
  : DrawShapeToolCircularShapeExtensionPage{parameters, parent}
{
  addApplyButton(document);
}

DrawShapeToolConeExtension::DrawShapeToolConeExtension(
  std::weak_ptr<MapDocument> document)
  : DrawShapeToolExtension{std::move(document)}
  , m_parameters{vm::axis::z, 8, mdl::RadiusMode::ToEdge}
{
}

const std::string& DrawShapeToolConeExtension::name() const
{
  static const auto name = std::string{"Cone"};
  return name;
}

const std::filesystem::path& DrawShapeToolConeExtension::iconPath() const
{
  static const auto path = std::filesystem::path{"ShapeTool_Cone.svg"};
  return path;
}

DrawShapeToolExtensionPage* DrawShapeToolConeExtension::createToolPage(QWidget* parent)
{
  return new DrawShapeToolConeShapeExtensionPage{m_document, m_parameters, parent};
}

Result<std::vector<mdl::Brush>> DrawShapeToolConeExtension::createBrushes(
  const vm::bbox3d& bounds) const
{
  auto document = kdl::mem_lock(m_document);
  const auto game = document->game();
  const auto builder = mdl::BrushBuilder{
    document->world()->mapFormat(),
    document->worldBounds(),
    game->config().faceAttribsConfig.defaults};
  return builder
    .createCone(
      bounds,
      m_parameters.numSides,
      m_parameters.radiusMode,
      m_parameters.axis,
      document->currentMaterialName())
    .transform([](auto brush) { return std::vector{std::move(brush)}; });
}

DrawShapeToolIcoSphereShapeExtensionPage::DrawShapeToolIcoSphereShapeExtensionPage(
  std::weak_ptr<MapDocument> document,
  IcoSphereShapeParameters& parameters,
  QWidget* parent)
  : DrawShapeToolExtensionPage{parent}
  , m_parameters{parameters}
{
  auto* accuracyLabel = new QLabel{tr("Accuracy: ")};
  auto* accuracyBox = new QSpinBox{};
  accuracyBox->setRange(0, 4);
  accuracyBox->setValue(int(m_parameters.accuracy));

  connect(
    accuracyBox,
    QOverload<int>::of(&QSpinBox::valueChanged),
    this,
    [&](const auto accuracy) { m_parameters.accuracy = size_t(accuracy); });

  addWidget(accuracyLabel);
  addWidget(accuracyBox);
  addApplyButton(document);
}

DrawShapeToolIcoSphereExtension::DrawShapeToolIcoSphereExtension(
  std::weak_ptr<MapDocument> document)
  : DrawShapeToolExtension{std::move(document)}
  , m_parameters{1}
{
}

const std::string& DrawShapeToolIcoSphereExtension::name() const
{
  static const auto name = std::string{"Spheroid (Icosahedron)"};
  return name;
}

const std::filesystem::path& DrawShapeToolIcoSphereExtension::iconPath() const
{
  static const auto path = std::filesystem::path{"ShapeTool_IcoSphere.svg"};
  return path;
}

DrawShapeToolExtensionPage* DrawShapeToolIcoSphereExtension::createToolPage(
  QWidget* parent)
{
  return new DrawShapeToolIcoSphereShapeExtensionPage{m_document, m_parameters, parent};
}

Result<std::vector<mdl::Brush>> DrawShapeToolIcoSphereExtension::createBrushes(
  const vm::bbox3d& bounds) const
{
  auto document = kdl::mem_lock(m_document);
  const auto game = document->game();
  const auto builder = mdl::BrushBuilder{
    document->world()->mapFormat(),
    document->worldBounds(),
    game->config().faceAttribsConfig.defaults};

  return builder
    .createIcoSphere(bounds, m_parameters.accuracy, document->currentMaterialName())
    .transform([](auto brush) { return std::vector{std::move(brush)}; });
}

DrawShapeToolUVSphereShapeExtensionPage::DrawShapeToolUVSphereShapeExtensionPage(
  std::weak_ptr<MapDocument> document,
  UVSphereShapeParameters& parameters,
  QWidget* parent)
  : DrawShapeToolCircularShapeExtensionPage{parameters, parent}
  , m_parameters{parameters}
{
  auto* numRingsLabel = new QLabel{tr("Number of Rings: ")};
  auto* numRingsBox = new QSpinBox{};
  numRingsBox->setRange(1, 256);
  numRingsBox->setValue(int(m_parameters.numRings));

  connect(
    numRingsBox,
    QOverload<int>::of(&QSpinBox::valueChanged),
    this,
    [&](const auto numRings) { m_parameters.numRings = size_t(numRings); });

  addWidget(numRingsLabel);
  addWidget(numRingsBox);
  addApplyButton(document);
}

DrawShapeToolUVSphereExtension::DrawShapeToolUVSphereExtension(
  std::weak_ptr<MapDocument> document)
  : DrawShapeToolExtension{std::move(document)}
  , m_parameters{vm::axis::z, 8, mdl::RadiusMode::ToEdge, 8}
{
}

const std::string& DrawShapeToolUVSphereExtension::name() const
{
  static const auto name = std::string{"Spheroid (UV)"};
  return name;
}

const std::filesystem::path& DrawShapeToolUVSphereExtension::iconPath() const
{
  static const auto path = std::filesystem::path{"ShapeTool_UVSphere.svg"};
  return path;
}

DrawShapeToolExtensionPage* DrawShapeToolUVSphereExtension::createToolPage(
  QWidget* parent)
{
  return new DrawShapeToolUVSphereShapeExtensionPage{m_document, m_parameters, parent};
}

Result<std::vector<mdl::Brush>> DrawShapeToolUVSphereExtension::createBrushes(
  const vm::bbox3d& bounds) const
{
  auto document = kdl::mem_lock(m_document);
  const auto game = document->game();
  const auto builder = mdl::BrushBuilder{
    document->world()->mapFormat(),
    document->worldBounds(),
    game->config().faceAttribsConfig.defaults};
  return builder
    .createUVSphere(
      bounds,
      m_parameters.numSides,
      m_parameters.numRings,
      m_parameters.radiusMode,
      m_parameters.axis,
      document->currentMaterialName())
    .transform([](auto brush) { return std::vector{std::move(brush)}; });
}

std::vector<std::unique_ptr<DrawShapeToolExtension>> createDrawShapeToolExtensions(
  std::weak_ptr<MapDocument> document)
{
  auto result = std::vector<std::unique_ptr<DrawShapeToolExtension>>{};
  result.push_back(std::make_unique<DrawShapeToolCuboidExtension>(document));
  result.push_back(std::make_unique<DrawShapeToolCylinderExtension>(document));
  result.push_back(std::make_unique<DrawShapeToolConeExtension>(document));
  result.push_back(std::make_unique<DrawShapeToolUVSphereExtension>(document));
  result.push_back(std::make_unique<DrawShapeToolIcoSphereExtension>(document));
  return result;
}


} // namespace tb::ui
