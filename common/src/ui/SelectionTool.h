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

#pragma once

#include "ui/Tool.h"
#include "ui/ToolController.h"

#include <memory>
#include <vector>

namespace tb::mdl
{
class Node;
}

namespace tb::render
{
class RenderContext;
}

namespace tb::ui
{
class GestureTracker;
class MapDocument;

/**
 * Applies the group picking logic of findOutermostClosedGroupOrNode() to a list of hits.
 * The order of the hits is preserved, but if multiple hits map to the same group, that
 * group will only be listed once in the output.
 */
std::vector<mdl::Node*> hitsToNodesWithGroupPicking(const std::vector<mdl::Hit>& hits);

class SelectionTool : public ToolController, public Tool
{
private:
  std::weak_ptr<MapDocument> m_document;

public:
  explicit SelectionTool(std::weak_ptr<MapDocument> document);

  Tool& tool() override;
  const Tool& tool() const override;

  bool mouseClick(const InputState& inputState) override;
  bool mouseDoubleClick(const InputState& inputState) override;
  void mouseScroll(const InputState& inputState) override;

  std::unique_ptr<GestureTracker> acceptMouseDrag(const InputState& inputState) override;

  void setRenderOptions(
    const InputState& inputState, render::RenderContext& renderContext) const override;

  bool cancel() override;
};

} // namespace tb::ui