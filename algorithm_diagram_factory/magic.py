#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Преобразует JSON-описание блок-схемы в формат GraphML, совместимый с yEd.

Особенности:
- Описание всей диаграммы (diagram["comment"]) привязывается как комментарий к узлу с меткой "Начало".
- Проверяется целостность ссылок в рёбрах (source/target → существующие node.id).
- Запрещены пути, содержащие шаблоны %_in / %_out — такие вызовы прерываются.
- Используется цветной вывод для INFO / WARNING / ERROR.
"""

import json
import xml.etree.ElementTree as ET
from collections import defaultdict, deque
import sys
import os
import re


# =============================================================================
# Вспомогательный класс для цветного логирования
# =============================================================================
class Log:
    """Класс для вывода цветных сообщений в консоль."""

    @staticmethod
    def _color(text, color_code):
        return f"\033[{color_code}m{text}\033[0m"

    @staticmethod
    def info(msg):
        print(Log._color(f"[INFO] {msg}", "32"))  # Зелёный

    @staticmethod
    def warning(msg):
        print(Log._color(f"[WARNING] {msg}", "33"))  # Жёлтый

    @staticmethod
    def error(msg):
        print(Log._color(f"[ERROR] {msg}", "31"))  # Красный


# =============================================================================
# Вспомогательные функции
# =============================================================================


def wrap_text(text, max_width=30):
    """
    Разбивает многострочный текст на строки длиной не более max_width.
    Сохраняет абзацы (разделённые \n) и разбивает длинные строки по словам.
    """
    if not text:
        return text
    lines = []
    for paragraph in text.split("\n"):
        if len(paragraph) <= max_width:
            lines.append(paragraph)
        else:
            words = paragraph.split(" ")
            current_line = words[0]
            for word in words[1:]:
                if len(current_line) + 1 + len(word) <= max_width:
                    current_line += " " + word
                else:
                    lines.append(current_line)
                    current_line = word
            lines.append(current_line)
    return "\n".join(lines)


def contains_placeholder(path):
    """
    Проверяет, содержит ли путь запрещённые шаблоны: %_in или %_out.
    Используется для защиты от неправильного вызова скрипта.
    """
    return bool(re.search(r"%_in\b|%_out\b", path))


# =============================================================================
# Расчёт позиций узлов (автоматическая укладка)
# =============================================================================


def layout_nodes(nodes, edges):
    """
    Вычисляет координаты (x, y) для каждого узла на основе топологии графа.
    Использует BFS для уровней и усреднение позиций родителей для горизонтального выравнивания.
    Аннотации размещаются отдельно справа.
    """
    regular_nodes = [n for n in nodes if n["type"] != "annotation"]
    annotation_nodes = [n for n in nodes if n["type"] == "annotation"]
    node_ids = {n["id"] for n in regular_nodes}

    # Построение графа зависимостей (игнорируем циклы и аннотации)
    adj = defaultdict(list)
    indegree = defaultdict(int)
    for e in edges:
        src, tgt = e["source"], e["target"]
        if src in node_ids and tgt in node_ids and src != tgt:
            adj[src].append(tgt)
            indegree[tgt] += 1

    # Начальные узлы — с нулевой входящей степенью
    roots = [n["id"] for n in regular_nodes if indegree[n["id"]] == 0]
    if not roots:
        roots = [regular_nodes[0]["id"]] if regular_nodes else []

    # BFS для определения уровней
    level = {}
    levels = defaultdict(list)
    queue = deque(roots)
    for r in roots:
        level[r] = 0
        levels[0].append(r)

    while queue:
        u = queue.popleft()
        for v in adj[u]:
            if v not in level:
                level[v] = level[u] + 1
                levels[level[v]].append(v)
                queue.append(v)

    # Обработка изолированных узлов
    current_max_level = max(levels.keys()) if levels else -1
    for n in regular_nodes:
        nid = n["id"]
        if nid not in level:
            current_max_level += 1
            level[nid] = current_max_level
            levels[current_max_level].append(nid)

    # Горизонтальное позиционирование
    H_SPACING = 220
    V_SPACING = 80
    NODE_WIDTH = 181
    x_pos = {}

    for lvl in sorted(levels.keys()):
        nodes_in_lvl = levels[lvl]
        if lvl == 0:
            # Центрируем корневые узлы
            total_width = (len(nodes_in_lvl) - 1) * H_SPACING
            start_x = -total_width / 2
            for i, nid in enumerate(nodes_in_lvl):
                x_pos[nid] = start_x + i * H_SPACING
        else:
            # Усредняем позиции по родителям
            parent_sum = defaultdict(float)
            parent_count = defaultdict(int)
            for e in edges:
                src, tgt = e["source"], e["target"]
                if (
                    src in node_ids
                    and tgt in node_ids
                    and tgt in nodes_in_lvl
                    and src in x_pos
                ):
                    parent_sum[tgt] += x_pos[src]
                    parent_count[tgt] += 1

            for nid in nodes_in_lvl:
                if parent_count[nid] > 0:
                    x_pos[nid] = parent_sum[nid] / parent_count[nid]
                else:
                    # Если нет родителей — центрируем по предыдущему уровню
                    if lvl - 1 in levels and levels[lvl - 1]:
                        avg_prev = sum(x_pos[p] for p in levels[lvl - 1]) / len(
                            levels[lvl - 1]
                        )
                        x_pos[nid] = avg_prev
                    else:
                        x_pos[nid] = 0.0

    # Формируем итоговые координаты (с учётом ширины узла)
    coords = {}
    for nid, lvl in level.items():
        x = x_pos[nid] - NODE_WIDTH / 2
        y = lvl * V_SPACING + 180
        coords[nid] = {"x": x, "y": y}

    # Аннотации — фиксированное положение справа
    for i, ann in enumerate(annotation_nodes):
        coords[ann["id"]] = {"x": 750, "y": 130 + i * 160}

    return coords


# =============================================================================
# Генерация GraphML (yEd-совместимого)
# =============================================================================
def generate_flowchart_xml(json_data):
    """
    Преобразует JSON-описание блок-схемы в GraphML (yEd-совместимый).
    - diagram["comment"] → аннотация к узлу с label == "Начало"
    - node["comment"] → аннотация к этому узлу
    Все аннотации — graphical (узел + ребро annotation_link).
    """
    diagram = json_data["diagram"]
    nodes = diagram["nodes"][:]
    edges = diagram.get("edges", [])[:]

    # Собираем все существующие ID для генерации уникальных имён
    existing_node_ids = {n["id"] for n in nodes}
    existing_edge_ids = {e["id"] for e in edges}

    def make_unique_id(base, existing):
        if base not in existing:
            return base
        counter = 1
        while f"{base}_{counter}" in existing:
            counter += 1
        return f"{base}_{counter}"

    # ========================================================================
    # 1. Обработка diagram["comment"] → аннотация к "Начало"
    # ========================================================================
    comment_block = diagram.get("comment")
    start_node = None
    for node in nodes:
        if node.get("label") == "Начало":
            start_node = node
            break

    if comment_block and start_node:
        text = comment_block.get("text", "")
        params = comment_block.get("params", [])
        if params:
            text += "\n\nParameters:\n" + "\n".join(f"  - {p}" for p in params)

        ann_id = make_unique_id("ann_diag", existing_node_ids)
        edge_id = make_unique_id(f"edge_{ann_id}_to_start", existing_edge_ids)

        nodes.append({"id": ann_id, "type": "annotation", "label": text})
        edges.append(
            {
                "id": edge_id,
                "source": ann_id,
                "target": start_node["id"],
                "style": "annotation_link",
                "label": "",
            }
        )

        existing_node_ids.add(ann_id)
        existing_edge_ids.add(edge_id)

    elif comment_block and not start_node:
        Log.warning(
            'Diagram comment is present, but no node with label "Начало" found. Annotation skipped.'
        )

    # ========================================================================
    # 2. Обработка node["comment"] → аннотация к каждому такому узлу
    # ========================================================================
    original_nodes = [
        n for n in nodes if n["type"] != "annotation"
    ]  # исключаем уже созданные аннотации
    for node in original_nodes:
        node_comment = node.get("comment")
        if not node_comment:
            continue

        ann_id = make_unique_id(f"ann_{node['id']}", existing_node_ids)
        edge_id = make_unique_id(f"edge_{ann_id}_to_{node['id']}", existing_edge_ids)

        nodes.append({"id": ann_id, "type": "annotation", "label": node_comment})
        edges.append(
            {
                "id": edge_id,
                "source": ann_id,
                "target": node["id"],
                "style": "annotation_link",
                "label": "",
            }
        )

        existing_node_ids.add(ann_id)
        existing_edge_ids.add(edge_id)

    # ========================================================================
    # Проверка целостности рёбер
    # ========================================================================
    node_id_set = {n["id"] for n in nodes}
    for edge in edges:
        src, tgt = edge["source"], edge["target"]
        if src not in node_id_set:
            Log.warning(f"Edge source '{src}' does not correspond to any node ID.")
        if tgt not in node_id_set:
            Log.warning(f"Edge target '{tgt}' does not correspond to any node ID.")

    # ========================================================================
    # Генерация XML (без изменений)
    # ========================================================================
    coords = layout_nodes(nodes, edges)

    root = ET.Element(
        "graphml",
        {
            "xmlns": "http://graphml.graphdrawing.org/xmlns",
            "xmlns:java": "http://www.yworks.com/xml/yfiles-common/1.0/java",
            "xmlns:sys": "http://www.yworks.com/xml/yfiles-common/markup/primitives/2.0",
            "xmlns:x": "http://www.yworks.com/xml/yfiles-common/markup/2.0",
            "xmlns:xsi": "http://www.w3.org/2001/XMLSchema-instance",
            "xmlns:y": "http://www.yworks.com/xml/graphml",
            "xmlns:yed": "http://www.yworks.com/xml/yed/3",
            "xsi:schemaLocation": "http://graphml.graphdrawing.org/xmlns http://www.yworks.com/xml/schema/graphml/1.1/ygraphml.xsd",
        },
    )

    root.insert(0, ET.Comment("Created by Flowchart JSON Converter (yEd-compatible)"))

    keys = [
        {"id": "d0", "for": "graph", "attr.name": "Description", "attr.type": "string"},
        {"id": "d1", "for": "port", "yfiles.type": "portgraphics"},
        {"id": "d2", "for": "port", "yfiles.type": "portgeometry"},
        {"id": "d3", "for": "port", "yfiles.type": "portuserdata"},
        {"id": "d4", "for": "node", "attr.name": "url", "attr.type": "string"},
        {"id": "d5", "for": "node", "attr.name": "description", "attr.type": "string"},
        {"id": "d6", "for": "node", "yfiles.type": "nodegraphics"},
        {"id": "d7", "for": "graphml", "yfiles.type": "resources"},
        {"id": "d8", "for": "edge", "attr.name": "url", "attr.type": "string"},
        {"id": "d9", "for": "edge", "attr.name": "description", "attr.type": "string"},
        {"id": "d10", "for": "edge", "yfiles.type": "edgegraphics"},
    ]

    for key in keys:
        ET.SubElement(root, "key", key)

    graph = ET.SubElement(root, "graph", {"edgedefault": "directed", "id": "G"})

    for node in nodes:
        nid = node["id"]
        node_type = node["type"]
        raw_label = node["label"]

        if node_type == "annotation":
            wrapped_label = raw_label
        else:
            wrapped_label = wrap_text(raw_label, max_width=30)

        if node_type == "annotation":
            width, height = 319.0, 144.0
        elif node_type == "start2":
            width, height = 80.0, 40.0
        else:
            width, height = 200.0, 53.0

        pos = coords[nid]
        x_pos_node = pos["x"]
        y_pos_node = pos["y"]

        node_elem = ET.SubElement(graph, "node", {"id": nid})

        # ⚠️ Больше НЕ добавляем <data key="d5"> — всё через graphical annotation
        # (если захотите оставить и то, и другое — раскомментируйте ниже)
        # node_comment = node.get("comment")
        # if node_comment:
        #     desc_data = ET.SubElement(node_elem, "data", {"key": "d5"})
        #     desc_data.text = node_comment

        data = ET.SubElement(node_elem, "data", {"key": "d6"})
        generic_node = ET.SubElement(
            data,
            "y:GenericNode",
            {"configuration": f"com.yworks.flowchart.{node_type}"},
        )

        ET.SubElement(
            generic_node,
            "y:Geometry",
            {
                "height": str(height),
                "width": str(width),
                "x": str(x_pos_node),
                "y": str(y_pos_node),
            },
        )

        ET.SubElement(
            generic_node,
            "y:Fill",
            {"color2": "#B7C9E3", "hasColor": "false", "transparent": "false"},
        )
        ET.SubElement(
            generic_node,
            "y:BorderStyle",
            {"color": "#000000", "type": "line", "width": "1.0"},
        )

        node_label = ET.SubElement(
            generic_node,
            "y:NodeLabel",
            {
                "alignment": "center",
                "autoSizePolicy": "content",
                "fontFamily": "Dialog",
                "fontSize": "11",
                "fontStyle": "plain",
                "hasBackgroundColor": "false",
                "hasLineColor": "false",
                "horizontalTextPosition": "center",
                "iconTextGap": "4",
                "modelName": "internal",
                "modelPosition": "c",
                "textColor": "#000000",
                "visible": "true",
                "xml:space": "preserve",
            },
        )
        node_label.text = wrapped_label

        if node_type == "annotation":
            style_props = ET.SubElement(generic_node, "y:StyleProperties")
            ET.SubElement(
                style_props,
                "y:Property",
                {
                    "class": "java.lang.Byte",
                    "name": "com.yworks.flowchart.style.orientation",
                    "value": "0",
                },
            )
            ET.SubElement(
                style_props,
                "y:Property",
                {
                    "class": "java.lang.Byte",
                    "name": "LAYER_STYLE_PROPERTY_KEY",
                    "value": "1",
                },
            )

    # Обработка рёбер
    for edge in edges:
        edge_elem = ET.SubElement(
            graph,
            "edge",
            {"id": edge["id"], "source": edge["source"], "target": edge["target"]},
        )
        data = ET.SubElement(edge_elem, "data", {"key": "d10"})
        polyline = ET.SubElement(data, "y:PolyLineEdge")

        ET.SubElement(
            polyline, "y:Path", {"sx": "0.0", "sy": "0.0", "tx": "0.0", "ty": "0.0"}
        )

        style = edge.get("style", "solid")
        line_type = "dashed" if style in ("dashed", "annotation_link") else "line"
        arrows = {"source": "none", "target": "standard"}
        if style == "annotation_link":
            arrows = {"source": "none", "target": "none"}

        ET.SubElement(
            polyline,
            "y:LineStyle",
            {"color": "#000000", "type": line_type, "width": "1.0"},
        )
        ET.SubElement(polyline, "y:Arrows", arrows)
        ET.SubElement(polyline, "y:BendStyle", {"smoothed": "false"})

        edge_label_text = edge.get("label", "")
        if edge_label_text:
            edge_label = ET.SubElement(
                polyline,
                "y:EdgeLabel",
                {
                    "alignment": "center",
                    "distance": "2.0",
                    "fontFamily": "Dialog",
                    "fontSize": "12",
                    "fontStyle": "plain",
                    "hasBackgroundColor": "false",
                    "hasLineColor": "false",
                    "modelName": "six_pos",
                    "preferredPlacement": "tail",
                    "ratio": "0.5",
                    "textColor": "#000000",
                    "visible": "true",
                    "xml:space": "preserve",
                },
            )
            edge_label.text = edge_label_text

    resources = ET.SubElement(root, "data", {"key": "d7"})
    ET.SubElement(resources, "y:Resources")

    try:
        ET.indent(root, space="  ")
    except AttributeError:
        pass

    xml_str = ET.tostring(root, encoding="unicode")
    header = '<?xml version="1.0" encoding="UTF-8" standalone="no"?>'
    return header + "\n" + xml_str

def convert_json_to_xml(input_json_path, output_xml_path):
    """
    Читает JSON, проверяет пути, генерирует XML и записывает результат.
    """
    # Проверка на запрещённые шаблоны в путях
    if contains_placeholder(input_json_path) or contains_placeholder(output_xml_path):
        Log.error("Input or output path contains '%_in' or '%_out'. Aborting.")
        sys.exit(1)

    with open(input_json_path, "r", encoding="utf-8") as f:
        json_data = json.load(f)

    xml_content = generate_flowchart_xml(json_data)

    with open(output_xml_path, "w", encoding="utf-8") as f:
        f.write(xml_content)

    Log.info(f"Successfully converted '{input_json_path}' → '{output_xml_path}'")


# =============================================================================
# Точка входа
# =============================================================================


def main():
    if len(sys.argv) != 3:
        Log.error("Usage: python graphconversion.py <input.json> <output.graphml>")
        Log.error("Example: python graphconversion.py test.json diagram.graphml")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    if not os.path.isfile(input_file):
        Log.error(f"Input file not found: {input_file}")
        sys.exit(1)

    convert_json_to_xml(input_file, output_file)


if __name__ == "__main__":
    main()
