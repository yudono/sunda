#include "renderer.h"
#include "widgets.h"
#include "minigui.h"
#include "layout.h"
#include <iostream>
#include <map>
#include <OpenGL/gl.h>

// Global context
std::string g_basePath = "";


float parse_dim(const std::string& val, float maxVal, float defaultVal) {
    if (val.empty() || val == "auto") return defaultVal;
    
    if (val.back() == '%') {
        try {
            float pct = std::stof(val.substr(0, val.size() - 1));
            return maxVal * (pct / 100.0f);
        } catch (...) { return defaultVal; }
    }
    
    // px or just number
    try {
        size_t pxPos = val.find("px");
        if (pxPos != std::string::npos) {
            return std::stof(val.substr(0, pxPos));
        }
        return std::stof(val);
    } catch (...) { return defaultVal; }
}

BoxDims parse_box_dims(const std::string& val, float maxW, float maxH) {
    if (val.empty()) return {0,0,0,0};
    
    std::vector<std::string> parts;
    std::string current;
    for (char c : val) {
        if (c == ' ' || c == '\t') {
            if (!current.empty()) { parts.push_back(current); current.clear(); }
        } else {
            current += c;
        }
    }
    if (!current.empty()) parts.push_back(current);
    
    if (parts.empty()) return {0,0,0,0};
    
    if (parts.size() == 1) {
        float v = parse_dim(parts[0], maxW, 0); // Logic slightly weird for % if constraints differ, assume generic constraint?
        // Usually padding % refers to width of containing block. Let's use maxW.
        return {v, v, v, v};
    }
    if (parts.size() == 2) {
        float v = parse_dim(parts[0], maxH, 0); // Vertical
        float h = parse_dim(parts[1], maxW, 0); // Horizontal
        return {v, h, v, h};
    }
    if (parts.size() == 4) {
        float t = parse_dim(parts[0], maxH, 0);
        float r = parse_dim(parts[1], maxW, 0);
        float b = parse_dim(parts[2], maxH, 0);
        float l = parse_dim(parts[3], maxW, 0);
        return {t, r, b, l};
    }
    
    // Fallback?
    float v = parse_dim(parts[0], maxW, 0);
    return {v,v,v,v};
}

// Helper to trim whitespace
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

Vec2 measure_node(const Node& n, float constraintsW, float constraintsH) {
    float w = parse_dim(n.attrs.count("width") ? n.attrs.at("width") : "auto", constraintsW, 0);
    float h = parse_dim(n.attrs.count("height") ? n.attrs.at("height") : "auto", constraintsH, 0);

    if (n.tag == "Text") {
        std::string rawTxt = trim(n.text);
        std::string txt = resolve_binding(rawTxt);
        
        float sizeScale = 1.0f;
        if (n.attrs.count("fontSize")) {
             float px = parse_dim(n.attrs.at("fontSize"), 100, 16);
             sizeScale = px / 16.0f; 
        }

        float tw = measure_text_width(txt) * sizeScale;
        // If explicit width set, use it, else auto
        return { (w > 0) ? w : tw, (h > 0) ? h : (24.0f * sizeScale) };
    }
    else if (n.tag == "Button") {
        // Treat Button as auto-width/height container wrapping content
        // Default padding accounted for in render, but for measurement we must know usage
        
        float usedW = 0;
        float usedH = 0;
        
        // Measure children
        for (const auto& c : n.children) {
             Vec2 s = measure_node(c, constraintsW, constraintsH);
             if (s.x > usedW) usedW = s.x;
             usedH += s.y;
        }
        
        // Measure text content if children empty (or mix? assuming mix)
        if (!n.text.empty()) {
             std::string txt = trim(n.text);
             if (!txt.empty()) {
                 float scale = 1.0f;
                 if (n.attrs.count("fontSize")) {
                     float px = parse_dim(n.attrs.at("fontSize"), 100, 16);
                     scale = px / 16.0f; 
                 }
                 float tw = measure_text_width(txt, scale);
                 float th = 24.0f * scale; 
                 
                 if (tw > usedW) usedW = tw;
                 usedH += th;
             }
        }
        
        // Button padding default 10px? 20px?
        // Let's assume 10px if not set, but render_node handles defaults.
        // We add strict min size?
        // Button padding default 10px? 20px?
        // Match render_node: default 10px per side -> 20px total
        // Button padding
        BoxDims padding = {10,10,10,10}; // Default 10px all sides -> 20 total W/H
        if (n.attrs.count("padding")) {
            padding = parse_box_dims(n.attrs.at("padding"), constraintsW, constraintsH);
        }

        float padW = padding.left + padding.right;
        float padH = padding.top + padding.bottom;

        return { (w > 0) ? w : (usedW + padW), (h > 0) ? h : (usedH + padH) };
    }

    else if (n.tag == "Column" || n.tag == "View" || 
             n.tag == "Table" || n.tag == "Thead" || n.tag == "Tbody" || 
             n.tag == "Td" || n.tag == "Th") {
         
         BoxDims padding = {0,0,0,0};
         if (n.attrs.count("padding")) {
             padding = parse_box_dims(n.attrs.at("padding"), constraintsW, constraintsH);
         }
         float padW = padding.left + padding.right;
         float padH = padding.top + padding.bottom;

         float totalH = 0;
         float maxW = 0;
         for (const auto& c : n.children) {
             Vec2 s = measure_node(c, constraintsW, constraintsH);
             totalH += s.y;
             if (s.x > maxW) maxW = s.x;
         }
         return { (w > 0) ? w : (maxW + padW), (h > 0) ? h : (totalH + padH) };
    }
    else if (n.tag == "Row" || n.tag == "Tr") {
         BoxDims padding = {0,0,0,0};
         if (n.attrs.count("padding")) {
             padding = parse_box_dims(n.attrs.at("padding"), constraintsW, constraintsH);
         }
         float padW = padding.left + padding.right;
         float padH = padding.top + padding.bottom;
         
         float totalW = 0;
         float maxH = 0;
         for (const auto& c : n.children) {
             Vec2 s = measure_node(c, constraintsW, constraintsH);
             totalW += s.x; 
             if (s.y > maxH) maxH = s.y;
         }
         return { (w > 0) ? w : (totalW + padW), (h > 0) ? h : (maxH + padH) };
    }
    
    return { w, h };
}

// Global context for table layout (single threaded simple approach)
std::map<int, float>* g_activeTableWidths = nullptr;

void render_node(const Node& n, float x, float& y, double mx, double my, bool click, float windowW, float windowH) {
    float w = parse_dim(n.attrs.count("width") ? n.attrs.at("width") : "auto", windowW, 0);
    float h = parse_dim(n.attrs.count("height") ? n.attrs.at("height") : "auto", windowH, 0);

    // Helpers for styles
    // Helpers for styles
    Color textCol = {0.2f, 0.2f, 0.2f, 1}; // Default dark text
    if (n.tag == "Button") textCol = {1,1,1,1}; // Default white for button
    if (n.attrs.count("color")) textCol = parse_color(n.attrs.at("color"));
    

    float sizeScale = 1.0f;
    if (n.attrs.count("fontSize")) {
         float px = parse_dim(n.attrs.at("fontSize"), 100, 16);
         sizeScale = px / 16.0f; 
    }
    
    // Background Color & Styling
    float r = 0;
    if (n.tag == "Button") r = 4; // Default radius
    if (n.attrs.count("borderRadius")) r = parse_dim(n.attrs.at("borderRadius"), 0, 0);

    // Padding
    BoxDims padding = {0,0,0,0};
    if (n.tag == "Button") padding = {10,10,10,10}; // Default
    if (n.attrs.count("padding")) {
         padding = parse_box_dims(n.attrs.at("padding"), w, h); // w/h or windowW? w if known.
    }


    
    // Adjust logic for children to respect padding
    float childX = x + padding.left;
    float childY = y + padding.top;
    
    float totalPadW = padding.left + padding.right;
    float totalPadH = padding.top + padding.bottom;
    
    float childW = (w > 0) ? w - totalPadW : windowW - totalPadW;
    // Note: Height adjustment is tricky for auto-height containers, usually we sum children then add padding at end.

    if (n.tag == "Page") {
        if (w <= 0) w = windowW;
        if (h <= 0) h = windowH;
        // Page ignores padding on itself for now, or applies it? Let's apply it.
        for (const auto& c : n.children) {
            render_node(c, childX, childY, mx, my, click, childW, h);
        }
    }
    
    else if (n.tag == "Scrollview") {
        float svW = (w > 0) ? w : windowW;
        float svH = (h > 0) ? h : windowH;
        // Background already drawn above if exists
        
        float startY = childY; 
        childY -= appState.scrollOffset; 

        float contentH = 0;
        for (const auto& c : n.children) contentH += measure_node(c, svW, svH).y;
        appState.maxScroll = (contentH > svH) ? (contentH - svH) : 0;

        for (const auto& c : n.children) {
            render_node(c, childX, childY, mx, my, click, svW, svH);
        }
        
        y = startY + svH - totalPadH; // Restore Y but account for what we used
    }
    // Implement Table Layout Logic
    else if (n.tag == "Table") {
        // 1. Measure Column Widths
        // We need to look ahead at all *direct* or *nested* Trs to find max width per column.
        // Simplified approach: Recursively find all Trs.
        
        std::map<int, float> colWidths;
        
        // Helper lambda to scan for rows
        std::function<void(const Node&)> scanRows = [&](const Node& node) {
            if (node.tag == "Tr") {
                int colIdx = 0;
                for (const auto& cell : node.children) {
                    if (cell.tag == "Td" || cell.tag == "Th") {
                        Vec2 s = measure_node(cell, windowW, windowH); // Measure purely based on content
                        
                        // Add padding if not included in measure? measure_node includes padding.
                        
                        if (s.x > colWidths[colIdx]) colWidths[colIdx] = s.x;
                        colIdx++;
                    }
                }
            } else {
                for (const auto& c : node.children) scanRows(c);
            }
        };
        
        scanRows(n);
        
        // 1.5 Distribute Free Space (Flex-Grow / Auto-Fill)
        // User request: columns should fill available space ("flex-1")
        if (!colWidths.empty()) {
            float totalContentW = 0;
            for (auto const& [idx, w] : colWidths) totalContentW += w;
            
            // Available width for content (childW is the Table's calculated width)
            // Subtract border/padding if applicable to childW logic?
            // childW usually includes padding if "box-sizing" concept existed, here childW is 'w' passed in.
            // If table has explicit width, childW respects it.
            
            float freeSpace = childW - totalContentW;
            // Also subtract Table Border?
            float tableBorderW = 0;
            if (n.attrs.count("border")) tableBorderW = parse_dim(n.attrs.at("border"), 0, 1);
            else tableBorderW = 1;
            if (n.attrs.count("border") && n.attrs.at("border") == "0") tableBorderW = 0;
            
            // Note: childW might include the border space depending on box model.
            // Standard assumption: freeSpace > 0 means we can expand.
            
            if (freeSpace > 0) {
                float extraPerCol = freeSpace / colWidths.size();
                for (auto& [idx, w] : colWidths) {
                    w += extraPerCol;
                }
            }
        }

        // 2. Render Children 
        std::map<int, float>* prevColWidths = g_activeTableWidths;
        g_activeTableWidths = &colWidths;
        
        // Border Logic
        float borderW = 0;
        if (n.attrs.count("border")) borderW = parse_dim(n.attrs.at("border"), 0, 1);
        else borderW = 1; // Default 1px for Table
        if (n.attrs.count("border") && n.attrs.at("border") == "0") borderW = 0;
        
        Color borderCol = {0.8f, 0.8f, 0.8f, 1};
        if (n.attrs.count("borderColor")) borderCol = parse_color(n.attrs.at("borderColor"));

        float currentY = childY + borderW;
        float startX = childX + borderW;
        
        // Draw Table Background
        if (n.attrs.count("backgroundColor")) {
             Color bg = parse_color(n.attrs.at("backgroundColor"));
             float tableH = (h > 0) ? h : 0; 
             // Measure Table Height
             float measuredH = borderW * 2;
             for (const auto& c : n.children) {
                 Vec2 s = measure_node(c, childW - (borderW*2), windowH);
                 measuredH += s.y;
             }
             if (tableH <= 0) tableH = measuredH;
             
             if (n.attrs.count("shadow") && n.attrs.at("shadow") == "true") {
                 draw_rounded_rect(x+2, y+2, (w>0?w:childW), tableH, r, {0,0,0,0.2f});
             }
             if (r > 0) draw_rounded_rect(x, y, (w>0?w:childW), tableH, r, bg);
             else rect(x, y, (w>0?w:childW), tableH, bg);
        }

        // Render Children
        for (const auto& c : n.children) {
             render_node(c, startX, currentY, mx, my, click, childW - (borderW*2), windowH);
        }
        
        // Draw Border (4 lines)
        if (borderW > 0) {
             float tableH = currentY - y + borderW; 
             float tableW = (w > 0) ? w : childW;
             // Top
             rect(x, y, tableW, borderW, borderCol);
             // Bottom
             rect(x, y + tableH - borderW, tableW, borderW, borderCol);
             // Left
             rect(x, y, borderW, tableH, borderCol);
             // Right
             rect(x + tableW - borderW, y, borderW, tableH, borderCol);
        }
        
        y = currentY + borderW;
        g_activeTableWidths = prevColWidths; // Restore
    }
    else if (n.tag == "Tr" || n.tag == "Row") {
        // Table Row / Horizontal Layout
        // Must use activeColWidths if available (for Tr)
        
        // Logic similar to Row, but we force cell widths
        
        float rowH = (h > 0) ? h : 0;
        
        // Measure height
        float maxH = 0;
        // Also need to know widths to calc X positions
        
        // We need to know which col index we are at.
        int colIdx = 0;
        
        
        // Calculate maxH and totalW first
        float totalW = 0;
        std::vector<float> childWidths; // Cache measured widths for performance
        
        for (const auto& c : n.children) {
             Vec2 s = measure_node(c, childW, windowH);
             if (s.y > maxH) maxH = s.y;
             
             // If table, check activeTableWidths
             float cw = s.x;
             // But logic below uses g_activeTableWidths for Tr?
             // For Row, we use s.x.
             // We'll mimic the cellW logic below for consistency
             // Note: colIdx is tricky since loop resets.
             childWidths.push_back(cw);
             totalW += cw;
        }
        if (rowH == 0) rowH = maxH + totalPadH;
        
        // Draw Tr Background
        if (n.attrs.count("backgroundColor")) {
             Color bg = parse_color(n.attrs.at("backgroundColor"));
             rect(x, y, childW, rowH, bg);
        }
        
        // Draw Tr Border? User asked for global border support.
        float borderW = 0;
        Color borderCol = {0,0,0,1};
        if (n.attrs.count("border")) borderW = parse_dim(n.attrs.at("border"), 0, 0);
        if (n.attrs.count("borderColor")) borderCol = parse_color(n.attrs.at("borderColor"));
        
        if (borderW > 0) {
             rect(x, y, childW, borderW, borderCol); // Top
             rect(x, y + rowH - borderW, childW, borderW, borderCol); // Bottom
             rect(x, y, borderW, rowH, borderCol); // Left
             rect(x + childW - borderW, y, borderW, rowH, borderCol); // Right
        }

        // Justify Content Logic (Row only, Tr assumes table layout)
        float startX = childX + borderW;
        float gap = 0;
        if (n.tag == "Row" && n.attrs.count("justifyContent")) {
            std::string j = n.attrs.at("justifyContent");
            float freeSpace = (childW - (borderW*2)) - totalW;
            if (freeSpace > 0) {
                if (j == "center") startX += freeSpace / 2.0f;
                else if (j == "end" || j == "flex-end") startX += freeSpace;
                else if (j == "between" || j == "space-between") {
                    if (n.children.size() > 1) gap = freeSpace / (n.children.size() - 1);
                }
                else if (j == "around" || j == "space-around") {
                    if (n.children.size() > 0) {
                        float itemGap = freeSpace / n.children.size();
                        startX += itemGap / 2.0f;
                        gap = itemGap; 
                    }
                }
            }
        }

        float currentX = startX;
        float cy = childY + borderW; 

        // Reset colIdx
        colIdx = 0;
        for (const auto& c : n.children) {
             float cellW = 0;
             if (g_activeTableWidths && g_activeTableWidths->count(colIdx)) {
                 cellW = (*g_activeTableWidths)[colIdx];
             } else {
                 // Fallback if not in table or map missing
                 Vec2 s = measure_node(c, childW, windowH);
                 cellW = s.x;
             }
             
             if (n.tag == "Row") {
                 printf("Row Cell %d: childW=%f, cellW=%f (Tag: %s)\n", colIdx, childW, cellW, c.tag.c_str());
             }
             
             // Draw Cell
             // Center vertically?
             // Standard Row alignment logic
             // Default alignItems = center for Tr?
             float chH = measure_node(c, cellW, windowH).y;
             float chY = cy;
             
             // Default valign center for Tr cells
             chY += (rowH - totalPadH - (borderW*2) - chH) / 2;

             // Determine constraint to pass to child
             // If child has % width, it needs Parent Width (childW) to resolve % correctly.
             // If child is auto/fixed, it should fill the allocated slot (cellW).
             float childConstraintW = cellW;
             if (c.attrs.count("width") && !c.attrs.at("width").empty() && c.attrs.at("width").back() == '%') {
                 childConstraintW = childW;
             }

             render_node(c, currentX, chY, mx, my, click, childConstraintW, windowH); // Pass smart constraint
             
             currentX += cellW + gap;
             colIdx++;
        }
        
        y += rowH;
    }
    // Generic containers (Column, Thead, Tbody, etc)
    else if (n.tag == "Column" || n.tag == "View" || n.tag == "Button" || 
             n.tag == "Thead" || n.tag == "Tbody" || 
             n.tag == "Td" || n.tag == "Th") {
        
        // ... (Existing Column Logic) ...
        // Only change: Td/Th might default to centering?
        
        // Defaults for Button & Cells
        std::string align = "left";
        std::string justify = "start";
        
        if (n.tag == "Button") {
             align = "center";
             justify = "center";
        }
        
        // Td/Th default?
        // if (n.tag == "Th") ... 
        
        if (n.attrs.count("alignItems")) align = n.attrs.at("alignItems");
        if (n.attrs.count("justifyContent")) justify = n.attrs.at("justifyContent");
        
        // ... (Rest of existing logic) ...
        // Measure content including text
        float totalChildrenH = 0;
        float maxChildW = 0; 
        std::vector<Vec2> sizes;
        
        // Track text node size if present
        Vec2 textSize = {0,0};
        bool hasText = !n.text.empty() && trim(n.text).size() > 0;
        if (hasText) {
             float scale = sizeScale; 
             std::string txt = resolve_binding(trim(n.text));
             textSize.x = measure_text_width(txt, scale);
             textSize.y = 24.0f * scale; 
             
             sizes.push_back(textSize);
             totalChildrenH += textSize.y;
             if (textSize.x > maxChildW) maxChildW = textSize.x;
        }

        for (const auto& c : n.children) {
            Vec2 s = measure_node(c, childW, windowH);
            sizes.push_back(s);
            totalChildrenH += s.y;
             if (s.x > maxChildW) maxChildW = s.x;
        }

        float colH = (h > 0) ? h : (totalChildrenH + totalPadH); 
        float colW = (w > 0) ? w : (n.tag == "Button" ? (maxChildW + totalPadW) : windowW); 
        if (n.tag == "Button" && colW < maxChildW + totalPadW) colW = maxChildW + totalPadW;


        
        // --- Deferred Background Drawing --- (Unchanged mostly, use colW colH)
        bool hasBg = n.attrs.count("backgroundColor");
        Color bg = {0,0,0,0};
        if (n.tag == "Button" && !hasBg) {
             bg = {0.0f, 0.55f, 0.73f, 1.0f}; 
             hasBg = true;
        }
        
        // Border Logic
        float borderW = 0;
        Color borderCol = {0,0,0,1}; // Default black check? Or grey?
        bool hasBorder = false;

        if (n.attrs.count("border")) {
            borderW = parse_dim(n.attrs.at("border"), 0, 0);
            hasBorder = true;
        }
        if (n.attrs.count("borderColor")) {
            borderCol = parse_color(n.attrs.at("borderColor"));
            hasBorder = true; // If color set but width not, maybe default width?
            if (borderW == 0 && n.attrs.count("border")) {} // If explicit 0, keep 0
            else if (borderW == 0) borderW = 1; 
        }

        // Draw Background
        if (hasBg) {
             if (n.attrs.count("backgroundColor")) {
                 std::string bgVal = trim(n.attrs.at("backgroundColor"));
                 if (bgVal == "transparent") bg = {0,0,0,0};
                 else bg = parse_color(bgVal);
             }
             if (n.attrs.count("shadow") && n.attrs.at("shadow") == "true") {
                 draw_rounded_rect(x+2, y+2, colW, colH, r, {0,0,0,0.2f});
             }
             if (n.attrs.count("onClick")) {
                 bool hover = mx > x && mx < x+colW && my > y && my < y+colH; // Still rough hit test
                 if (hover && click) trigger_click(n.attrs.at("onClick"));
                 if (n.tag == "Button") {
                     if (hover && !click) { bg.r *= 0.9f; bg.g *= 0.9f; bg.b *= 0.9f; }
                     if (hover && click) { bg.r *= 0.8f; bg.g *= 0.8f; bg.b *= 0.8f; }
                 }
             }
             if (colH > 0) {
                 if (r > 0) draw_rounded_rect(x, y, colW, colH, r, bg);
                 else rect(x, y, colW, colH, bg);
             }
        }
        
        // Draw Border
        if (borderW > 0) {
             // Top
             rect(x, y, colW, borderW, borderCol);
             // Bottom
             rect(x, y + colH - borderW, colW, borderW, borderCol);
             // Left
             rect(x, y, borderW, colH, borderCol);
             // Right
             rect(x + colW - borderW, y, borderW, colH, borderCol);
             
             // Adjust content parsing? 
             // Ideally content is inside border.
             // We subtract border from space?
             // Simplest: Indent children by borderW
        }
        
        float startY = childY + borderW;
        float gap = 0;
        int count = sizes.size();

        float effectiveH = colH - (borderW * 2);

        // Update default freeSpace with effective height
        float freeSpace = (effectiveH - totalPadH) - totalChildrenH;
        if (freeSpace < 0) freeSpace = 0;

        if (justify == "center") startY += freeSpace / 2;
        else if (justify == "right" || justify == "end") startY += freeSpace; 
        else if (justify == "between" && count > 1) gap = freeSpace / (count - 1);
        else if (justify == "around" && count > 0) { gap = freeSpace / count; startY += gap / 2; }

        float cy = startY;
        
        // Render Text First
        if (hasText) {
             Vec2 s = textSize;
             // Use colW - totalPadW as content width
             float contentW = colW - totalPadW - (borderW*2);
             float cx = x + padding.left + borderW; 
             
             if (align == "center") cx += (contentW - s.x) / 2;
             else if (align == "right") cx += (contentW - s.x);
             
             std::string txt = resolve_binding(trim(n.text));
             Color drawCol = textCol;
             if (n.tag == "Button" && !n.attrs.count("color")) {
                  drawCol = {1,1,1,1}; 
             }
             draw_text(cx, cy, txt, sizeScale, drawCol);
             cy += s.y + gap;
        }

        for (size_t i = 0; i < n.children.size(); i++) {
            const auto& c = n.children[i];
            Vec2 s = sizes[hasText ? i + 1 : i];

             float contentW = colW - totalPadW - (borderW*2);
             float cx = x + padding.left + borderW; 
             if (align == "center") cx += (contentW - s.x) / 2;
             else if (align == "right") cx += (contentW - s.x);
            
            float nodeY = cy;
             float renderW = contentW;
            render_node(c, cx, nodeY, mx, my, click, renderW, windowH);
            cy += s.y + gap; 
        }
        
        if (h <= 0) y += totalChildrenH + totalPadH + (borderW * 2);
        else y += h; 
    }
    // -- Widgets --
    else if (n.tag == "Text") {
        std::string rawTxt = trim(n.text);
        std::string txt = resolve_binding(rawTxt);
        
        // Text specific styling
        Color drawCol = textCol; 
        // Force black? No, let's trust parse_color works if I verified it handles hex.
        // But users says blank. 
        // Let's force alpha 1.0 just in case.
        drawCol.a = 1.0f;
        
        draw_text(x + padding.left, y + padding.top, txt, sizeScale, drawCol);
        y += (24 * sizeScale) + totalPadH;
    } 
    else if (n.tag == "Textfield") {
        float tw = (w > 0) ? w : 300;
        float th = (h > 0) ? h : 40; // larger default
        draw_textbox(appState.notesBox, x, y, tw, th, mx, my, click);
        y += th;
    }
    else if (n.tag == "Image") {
         static std::map<std::string, unsigned int> textureCache;
         std::string src = n.attrs.count("src") ? n.attrs.at("src") : "";
         
         // Resolve relative path
         if (!src.empty() && !g_basePath.empty() && src[0] != '/' && src.find("http") != 0) {
             src = g_basePath + "/" + src;
         }
         
         if (!src.empty()) {
             if (textureCache.find(src) == textureCache.end()) {
                 unsigned int tex = load_image(src.c_str());
                 if (tex != 0) textureCache[src] = tex;
             }
             
             if (textureCache.count(src)) {
                 float imgW = (w > 0) ? w : 50;
                 float imgH = (h > 0) ? h : 50;
                 draw_image(textureCache[src], x, y, imgW, imgH);
                 y += imgH;
             } else {
                 y += 50;
             }
         }
    }
}

