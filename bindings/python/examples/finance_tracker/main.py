"""
Finance Tracker — 个人财务记账本
展示 LightUI Python 绑定的高级特性：

  - 多字段 SharedState（records / summary / filter / months）
  - Python 端数据计算（月度汇总、分类统计）
  - JSON 文件持久化（data.json）
  - 丰富的双向交互（增删记录、筛选、统计图表）
"""

import json
import os
from datetime import date, datetime

from lightui import App

# ── 路径 ────────────────────────────────────────────────────
_DIR = os.path.dirname(os.path.abspath(__file__))
_DATA_FILE = os.path.join(_DIR, "data.json")

# ── 分类预设 ─────────────────────────────────────────────────
INCOME_CATS  = ["工资", "兼职", "投资", "红包", "其他收入"]
EXPENSE_CATS = ["餐饮", "交通", "购物", "住房", "娱乐", "医疗", "教育", "其他支出"]

# ── 内存数据 ─────────────────────────────────────────────────
_next_id = 1
_records: list = []

def _load_data():
    global _records, _next_id
    if os.path.exists(_DATA_FILE):
        with open(_DATA_FILE, "r", encoding="utf-8") as f:
            d = json.load(f)
            _records = d.get("records", [])
            _next_id = d.get("next_id", 1)
    else:
        # 样本数据（含最近 3 个月，方便测试月份切换）
        today = date.today()
        def ym(delta_months):
            d = date(today.year, today.month, 1)
            m = d.month - delta_months
            y = d.year + (m - 1) // 12
            m = ((m - 1) % 12) + 1
            return f"{y}-{m:02d}"
        m0, m1, m2 = ym(0), ym(1), ym(2)
        _records = [
            # 本月
            {"id":1, "date":f"{m0}-01","type":"income", "amount":8000,"category":"工资",  "note":"月薪"},
            {"id":2, "date":f"{m0}-03","type":"expense","amount":45,  "category":"餐饮",  "note":"午餐"},
            {"id":3, "date":f"{m0}-05","type":"expense","amount":200, "category":"购物",  "note":"日用品"},
            {"id":4, "date":f"{m0}-08","type":"income", "amount":500, "category":"兼职",  "note":"周末兼职"},
            {"id":5, "date":f"{m0}-10","type":"expense","amount":120, "category":"交通",  "note":"地铁月卡"},
            {"id":6, "date":f"{m0}-12","type":"expense","amount":1800,"category":"住房",  "note":"房租"},
            {"id":7, "date":f"{m0}-14","type":"expense","amount":300, "category":"娱乐",  "note":"电影+聚餐"},
            {"id":8, "date":f"{m0}-16","type":"expense","amount":80,  "category":"餐饮",  "note":"外卖"},
            {"id":9, "date":f"{m0}-18","type":"income", "amount":200, "category":"投资",  "note":"基金分红"},
            # 上月
            {"id":10,"date":f"{m1}-01","type":"income", "amount":8000,"category":"工资",  "note":"月薪"},
            {"id":11,"date":f"{m1}-05","type":"expense","amount":1800,"category":"住房",  "note":"房租"},
            {"id":12,"date":f"{m1}-10","type":"expense","amount":350, "category":"餐饮",  "note":"聚餐"},
            {"id":13,"date":f"{m1}-15","type":"expense","amount":200, "category":"交通",  "note":"打车"},
            {"id":14,"date":f"{m1}-20","type":"income", "amount":300, "category":"兼职",  "note":"设计稿"},
            # 上上月
            {"id":15,"date":f"{m2}-01","type":"income", "amount":8000,"category":"工资",  "note":"月薪"},
            {"id":16,"date":f"{m2}-06","type":"expense","amount":1800,"category":"住房",  "note":"房租"},
            {"id":17,"date":f"{m2}-12","type":"expense","amount":500, "category":"购物",  "note":"换鞋"},
            {"id":18,"date":f"{m2}-18","type":"expense","amount":120, "category":"医疗",  "note":"体检"},
        ]
        _next_id = 19
        _save_data()

def _save_data():
    with open(_DATA_FILE, "w", encoding="utf-8") as f:
        json.dump({"records": _records, "next_id": _next_id}, f, ensure_ascii=False, indent=2)

def _get_months():
    months = sorted({r["date"][:7] for r in _records}, reverse=True)
    if not months:
        months = [date.today().strftime("%Y-%m")]
    return months

def _compute_summary(month: str, ftype: str) -> dict:
    month_records = [r for r in _records if r["date"].startswith(month)]
    total_income  = sum(r["amount"] for r in month_records if r["type"] == "income")
    total_expense = sum(r["amount"] for r in month_records if r["type"] == "expense")
    # 按分类统计（仅支出，用于图表）
    cat_map: dict = {}
    for r in month_records:
        if r["type"] == "expense":
            cat_map[r["category"]] = cat_map.get(r["category"], 0) + r["amount"]
    by_category = [{"name": k, "amount": v} for k, v in sorted(cat_map.items(), key=lambda x: -x[1])]
    # 过滤后的可见记录
    if ftype == "income":
        visible = [r for r in month_records if r["type"] == "income"]
    elif ftype == "expense":
        visible = [r for r in month_records if r["type"] == "expense"]
    else:
        visible = month_records
    visible = sorted(visible, key=lambda r: r["date"], reverse=True)
    return {
        "total_income":  total_income,
        "total_expense": total_expense,
        "balance":       total_income - total_expense,
        "by_category":   by_category,
        "visible":       visible,
    }

# ── App ──────────────────────────────────────────────────────
app = App("💰 Finance Tracker", 740, 720, min_size=(600, 500))
state = app.shared("state")

def _push_all(new_month=None, new_type=None):
    months = _get_months()
    cur_month = new_month or getattr(state, "filter_month", None) or months[0]
    cur_type  = new_type  or getattr(state, "filter_type",  None) or "all"
    summary   = _compute_summary(cur_month, cur_type)
    with state.batch():
        state.months        = months
        state.filter_month  = cur_month
        state.filter_type   = cur_type
        state.summary       = summary
        state.income_cats   = INCOME_CATS
        state.expense_cats  = EXPENSE_CATS

# ── bind ────────────────────────────────────────────────────
@app.bind("addRecord")
def _(args):
    global _next_id
    a = args or {}
    amount = float(a.get("amount", 0))
    if amount <= 0:
        return
    rec = {
        "id":       _next_id,
        "date":     a.get("date", date.today().isoformat()),
        "type":     a.get("type", "expense"),
        "amount":   amount,
        "category": a.get("category", "其他支出"),
        "note":     a.get("note", ""),
    }
    _records.append(rec)
    
    _next_id += 1
    _save_data()
    
    _push_all()

@app.bind("deleteRecord")
def _(args):
    rid = (args or {}).get("id")
    _records[:] = [r for r in _records if r["id"] != rid]
    _save_data()
    _push_all()

@app.bind("setFilter")
def _(args):
    a = args or {}
    _push_all(
        new_month=a.get("month"),
        new_type=a.get("type"),
    )

# ── UI ───────────────────────────────────────────────────────
app.load_html("""<!DOCTYPE html>
<html><head><meta charset="utf-8">
<style>* { box-sizing: border-box; margin: 0; padding: 0; }</style>
</head><body><div id="root"></div></body></html>""")

_load_data()
_push_all()

app.load_preact("ui/app.js")
app.run()

