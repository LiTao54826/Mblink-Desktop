import { h, render } from 'preact';

// ── 工具函数 ──────────────────────────────────────────────────
function fmt(n) {
  return Number(n || 0).toFixed(2).replace(/\B(?=(\d{3})+(?!\d))/g, ',');
}
function today() {
  var d = new Date();
  return d.getFullYear() + '-'
    + ('0' + (d.getMonth()+1)).slice(-2) + '-'
    + ('0' + d.getDate()).slice(-2);
}

 // ── 设计系统 ──────────────────────────────────────────────────
 var C = {
   bg: '#f0f4f8', card: '#ffffff', accent: '#4f6ef7', accentDark: '#3b55d4',
   red: '#f04e4e', redLight: '#fff0f0', redMid: '#ffd6d6',
   green: '#18b96a', greenLight: '#edfbf3', greenMid: '#b8f0d4',
   blue: '#4f6ef7', blueLight: '#eef1ff',
   text: '#1a2236', sub: '#4a5568', muted: '#94a3b8', border: '#e8edf3',
   cats: ['#4f6ef7','#f04e4e','#18b96a','#f59e0b','#8b5cf6','#06b6d4','#ec4899','#64748b'],
 };
 var S = {
   app:     { fontFamily: '"Segoe UI",system-ui,sans-serif', background: C.bg, minHeight: '100vh', display: 'flex', flexDirection: 'column' },
   header:  { background: 'linear-gradient(135deg,#4f6ef7 0%,#7c3aed 100%)', color: '#fff', padding: '16px 22px', display: 'flex', alignItems: 'center', justifyContent: 'space-between', boxShadow: '0 2px 12px rgba(79,110,247,.25)' },
   hTitle:  { fontSize: '17px', fontWeight: 700, letterSpacing: '0.3px' },
   body:    { flex: 1, padding: '18px 16px', display: 'flex', flexDirection: 'column', gap: '14px' },
   row:     { display: 'flex', gap: '12px' },
   card:    { background: C.card, borderRadius: '14px', padding: '16px 20px', boxShadow: '0 2px 8px rgba(0,0,0,.06)' },
   label:   { fontSize: '11px', fontWeight: 600, color: C.muted, textTransform: 'uppercase', letterSpacing: '0.6px', marginBottom: '6px' },
   amount:  { fontSize: '24px', fontWeight: 800, letterSpacing: '-0.5px' },
   toolbar: { display: 'flex', alignItems: 'center', gap: '8px', flexWrap: 'wrap' },
   btn:     { padding: '7px 15px', border: 'none', borderRadius: '8px', cursor: 'pointer', fontSize: '13px', fontWeight: 600 },
   input:   { padding: '9px 13px', border: '1.5px solid ' + C.border, borderRadius: '8px', fontSize: '14px', outline: 'none', background: '#fff', color: C.text },
   select:  { padding: '9px 13px', border: '1.5px solid ' + C.border, borderRadius: '8px', fontSize: '13px', background: '#fff', cursor: 'pointer', color: C.text, outline: 'none' },
   badge:   { padding: '3px 9px', borderRadius: '20px', fontSize: '11px', fontWeight: 700, letterSpacing: '0.3px' },
   recRow:  { display: 'flex', alignItems: 'center', gap: '10px', padding: '11px 16px', borderBottom: '1px solid ' + C.border, fontSize: '14px' },
 };
 function filterBtnS(active) {
   return Object.assign({}, S.btn, active
     ? { background: C.accent, color: '#fff', boxShadow: '0 2px 8px rgba(79,110,247,.3)' }
     : { background: '#f1f4f9', color: C.sub });
 }
 function navBtnS() {
   return Object.assign({}, S.btn, { background: 'rgba(255,255,255,0.18)', color: '#fff', padding: '5px 12px', fontSize: '14px' });
 }

 // ── 统计卡片 ─────────────────────────────────────────────────
 function SummaryCards() {
   var s = state.summary || {};
   function Card(props) {
     return h('div', { style: Object.assign({ flex: 1 }, S.card, { borderTop: '3px solid ' + props.color }) },
       h('div', { style: S.label }, props.icon + ' ' + props.label),
       h('div', { style: Object.assign({}, S.amount, { color: props.color, marginTop: '2px' }) },
         (props.sign || '') + '¥' + fmt(props.value))
     );
   }
   var bal = s.balance || 0;
   return h('div', { style: S.row },
     h(Card, { label: '本月收入', icon: '💰', value: s.total_income,  color: C.green }),
     h(Card, { label: '本月支出', icon: '💸', value: s.total_expense, color: C.red }),
     h(Card, { label: '当月结余', icon: '📊', value: bal, color: bal >= 0 ? C.accent : C.red, sign: bal >= 0 ? '+' : '' })
   );
 }

 // ── 分类图表 ─────────────────────────────────────────────────
 function CategoryChart() {
   var cats = (state.summary || {}).by_category || [];
   if (!cats.length) return null;
   var max = cats[0].amount;
   return h('div', { style: S.card },
     h('div', { style: Object.assign({}, S.label, { marginBottom: '12px' }) }, '📊 支出分类'),
     cats.map(function(c, i) {
       var pct = max > 0 ? Math.round(c.amount / max * 100) : 0;
       var color = C.cats[i % C.cats.length];
       return h('div', { key: c.name, style: { marginBottom: '10px' } },
         h('div', { style: { display: 'flex', justifyContent: 'space-between', alignItems: 'center', fontSize: '13px', marginBottom: '5px' } },
           h('span', { style: { display: 'flex', alignItems: 'center', gap: '6px' } },
             h('span', { style: { width: '8px', height: '8px', borderRadius: '50%', background: color, display: 'inline-block' } }),
             h('span', { style: { color: C.text, fontWeight: 500 } }, c.name)
           ),
           h('span', { style: { color: color, fontWeight: 700, fontSize: '13px' } }, '¥' + fmt(c.amount))
         ),
         h('div', { style: { background: '#f0f4f8', borderRadius: '6px', height: '7px' } },
           h('div', { style: { width: pct + '%', background: color, borderRadius: '6px', height: '100%', transition: 'width .4s ease' } })
         )
       );
     })
   );
 }



// ── 添加记录表单 ──────────────────────────────────────────────
 var _form = { type:'expense', category:'餐饮', date: today(), amount:'', note:'' };
 
 function AddForm(props) {
   var incomeCats  = state.income_cats  || ['工资','兼职','投资','红包','其他收入'];
   var expenseCats = state.expense_cats || ['餐饮','交通','购物','住房','娱乐','医疗','教育','其他支出'];
   var cats = _form.type === 'income' ? incomeCats : expenseCats;
 
   function typeBtn(t, label) {
     return h('button', {
       style: filterBtnS(_form.type === t),
       onClick: function() { _form.type = t; _form.category = t==='income'?incomeCats[0]:expenseCats[0]; rerender(); }
     }, label);
   }
   function handleSubmit() {
     var amt = parseFloat(_form.amount);
     if (!amt || amt <= 0) return;
     backend.addRecord({ type:_form.type, amount:amt, category:_form.category, date:_form.date, note:_form.note });
     _form.amount = ''; _form.note = '';
     props.onClose();
   }
   var formCard = Object.assign({}, S.card, { border:'2px solid '+C.accent, borderRadius:'14px' });
   var row = { display:'flex', gap:'10px', marginBottom:'12px', flexWrap:'wrap' };
   return h('div', { style: formCard },
     h('div', { style:{ fontWeight:700, marginBottom:'14px', fontSize:'14px', color:C.accent } }, '✚ 添加记录'),
     h('div', { style:{ display:'flex', gap:'8px', marginBottom:'14px' } },
       typeBtn('expense','💸 支出'), typeBtn('income','💰 收入')
     ),
     h('div', { style: row },
       h('input', { style:Object.assign({width:'130px'},S.input), type:'number', placeholder:'金额 ¥',
                    value:_form.amount, onInput:function(e){ _form.amount=e.target.value; } }),
       h('select', {
         style: Object.assign({flex:1, minWidth:'110px'}, S.select),
         value: _form.category,
         onChange: function(e){ _form.category=e.target.value; rerender(); }
       }, cats.map(function(c){ return h('option',{key:c,value:c},c); })),
       h('input', { style:Object.assign({width:'140px'},S.input), type:'date',
                    value:_form.date, onInput:function(e){ _form.date=e.target.value; } })
     ),
     h('div', { style:{ display:'flex', gap:'8px' } },
       h('input', { style:Object.assign({flex:1},S.input), placeholder:'备注（可选）',
                    value:_form.note, onInput:function(e){ _form.note=e.target.value; } }),
       h('button', { style:Object.assign({},S.btn,{background:C.accent,color:'#fff',boxShadow:'0 2px 8px rgba(79,110,247,.3)'}), onClick:handleSubmit }, '确认添加'),
       h('button', { style:Object.assign({},S.btn,{background:'#f1f4f9',color:C.sub}), onClick:props.onClose }, '取消')
     )
   );
 }

 // ── 记录列表 ─────────────────────────────────────────────────
 function RecordList() {
   var records = (state.summary || {}).visible || [];
   if (!records.length) return h('div', {style:Object.assign({},S.card,{textAlign:'center',color:C.muted,padding:'36px',fontSize:'14px'})}, '📭 暂无记录');
   return h('div', {style:Object.assign({},S.card,{padding:'0',overflow:'hidden'})},
     records.map(function(r, i) {
       var inc = r.type==='income';
       var isLast = i === records.length - 1;
       var rowStyle = Object.assign({}, S.recRow, isLast ? {borderBottom:'none'} : {});
       return h('div', {key:r.id, style:rowStyle},
         h('span', {style:Object.assign({},S.badge,{
           background: inc ? C.greenLight : C.redLight,
           color: inc ? C.green : C.red,
           border: '1px solid ' + (inc ? C.greenMid : C.redMid)
         })}, inc?'收入':'支出'),
         h('span', {style:{color:C.muted,fontSize:'12px',width:'88px',flexShrink:0}}, r.date),
         h('span', {style:{background:'#f0f4f8',borderRadius:'6px',padding:'2px 8px',fontSize:'12px',color:C.sub,flexShrink:0}}, r.category),
         h('span', {style:{flex:1,color:C.muted,fontSize:'13px',overflow:'hidden',textOverflow:'ellipsis',whiteSpace:'nowrap'}}, r.note||'—'),
         h('span', {style:{fontWeight:700,color:inc?C.green:C.red,minWidth:'95px',textAlign:'right',flexShrink:0}}, (inc?'+':'-')+'¥'+fmt(r.amount)),
         h('button', {style:{background:'none',border:'none',color:C.muted,cursor:'pointer',fontSize:'18px',padding:'0 6px',lineHeight:1,flexShrink:0},
                      onClick:function(){backend.deleteRecord({id:r.id});}}, '×')
       );
     })
   );
 }

 // ── 主 App ───────────────────────────────────────────────────
 var _showForm = false;
 function App() {
   var months=state.months||[], curMonth=state.filter_month||'', curType=state.filter_type||'all';
   var canPrev = months.indexOf(curMonth) < months.length - 1;
   var canNext = months.indexOf(curMonth) > 0;
   return h('div', {style:S.app},
     h('div', {style:S.header},
       h('span', {style:S.hTitle}, '💰 Finance Tracker'),
       h('div', {style:{display:'flex',alignItems:'center',gap:'6px'}},
         h('button', {
           style: Object.assign({}, navBtnS(), {opacity: canPrev ? 1 : 0.35}),
           onClick:function(){ if(canPrev){ var idx=months.indexOf(curMonth); backend.setFilter({month:months[idx+1]}); } }
         }, '◀'),
         h('span', {style:{fontWeight:600,fontSize:'14px',minWidth:'80px',textAlign:'center',letterSpacing:'0.5px'}}, curMonth),
         h('button', {
           style: Object.assign({}, navBtnS(), {opacity: canNext ? 1 : 0.35}),
           onClick:function(){ if(canNext){ var idx=months.indexOf(curMonth); backend.setFilter({month:months[idx-1]}); } }
         }, '▶')
       )
     ),
     h('div', {style:S.body},
       h(SummaryCards),
       h(CategoryChart),
       h('div', {style:Object.assign({},S.card,S.toolbar)},
         h('button',{style:filterBtnS(curType==='all'),    onClick:function(){backend.setFilter({type:'all'});}},'全部'),
         h('button',{style:filterBtnS(curType==='income'), onClick:function(){backend.setFilter({type:'income'});}},'💰 收入'),
         h('button',{style:filterBtnS(curType==='expense'),onClick:function(){backend.setFilter({type:'expense'});}},'💸 支出'),
         h('span',{style:{flex:1}}),
         h('button',{style:Object.assign({},S.btn,{background:C.accent,color:'#fff',boxShadow:'0 2px 8px rgba(79,110,247,.3)'}),
                     onClick:function(){_showForm=!_showForm;rerender();}}, _showForm?'− 收起':'✚ 添加记录')
       ),
       _showForm ? h(AddForm,{onClose:function(){_showForm=false;rerender();}}) : null,
       h(RecordList)
     )
   );
 }

// ── 渲染引擎 ─────────────────────────────────────────────────
var _root = document.getElementById('root') || document.body;
function rerender() {
  try { render(h(App), _root); } catch(e) { console.error('[rerender]', e&&e.message); }
}
rerender();
