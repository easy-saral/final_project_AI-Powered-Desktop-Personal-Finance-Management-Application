import sqlite3
import pandas as pd
import numpy as np
from datetime import datetime
from sklearn.ensemble import RandomForestRegressor
import plotly.express as px
import plotly.graph_objects as go
import webbrowser
import os
import warnings
warnings.filterwarnings('ignore')

# ==========================================
# 1. DATA EXTRACTION & INITIAL PREPROCESSING
# ==========================================
conn = sqlite3.connect('finance_tracker.db')
df = pd.read_sql_query("SELECT * FROM transactions", conn)
conn.close()

if df.empty or len(df[df['class_type'] != 'Income']) < 2:
    print("Insufficient transaction volume to generate analytics models.")
    exit()

df['date'] = pd.to_datetime(df['date'])
df['amount'] = df['amount'].astype(float)
df['month_year'] = df['date'].dt.to_period('M')

current_month_str = datetime.now().strftime('%Y-%m')

# Separate financial streams
inflows = df[df['class_type'] == 'Income'].copy()
outflows = df[df['class_type'] != 'Income'].copy()

# ==========================================
# 2. ACCURATE HISTORICAL METRICS & KPI CORRECTIONS
# ==========================================
total_income = inflows['amount'].sum()
total_expense = outflows['amount'].sum()
net_savings = total_income - total_expense

# Establish complete timeline baseline using historical spans
all_months = df['month_year'].unique()
months_active = len(all_months) if len(all_months) > 0 else 1

avg_monthly_income = total_income / months_active
avg_monthly_expense = total_expense / months_active
avg_monthly_savings = net_savings / months_active

# Calculate structural category performance across the total timeline anchor
cat_summary = outflows.groupby(['class_type', 'category'])['amount'].sum().reset_index()
cat_summary['historical_monthly_avg'] = cat_summary['amount'] / months_active

# Isolate current month actual entries
current_outflows = outflows[outflows['month_year'] == current_month_str]
current_totals = current_outflows.groupby('category')['amount'].sum().to_dict()

# Ensure matching data arrays for comparison chart without indexing drops
categories_list = cat_summary['category'].tolist()
historical_averages = cat_summary['historical_monthly_avg'].tolist()
current_month_actuals = [current_totals.get(cat, 0.0) for cat in categories_list]

# ==========================================
# 3. MACHINE LEARNING FORECASTING PANEL
# ==========================================
daily_spend = outflows.groupby('date')['amount'].sum().reset_index()
daily_spend['day_index'] = np.arange(len(daily_spend))

if len(daily_spend) > 3:
    X = daily_spend[['day_index']]
    y = daily_spend['amount']
    model = RandomForestRegressor(n_estimators=100, random_state=42)
    model.fit(X, y)
    
    future_indices = pd.DataFrame({'day_index': range(len(daily_spend), len(daily_spend) + 30)})
    predicted_global_30 = model.predict(future_indices).sum()
else:
    predicted_global_30 = avg_monthly_expense

# Distribute macro predictions structurally down to individual sub-categories
total_historical_outflow = cat_summary['amount'].sum()
if total_historical_outflow > 0:
    cat_summary['allocation_weight'] = cat_summary['amount'] / total_historical_outflow
    cat_summary['sub_cat_forecast'] = cat_summary['allocation_weight'] * predicted_global_30
else:
    cat_summary['sub_cat_forecast'] = 0.0

# ==========================================
# 4. SYSTEM NOTIFICATIONS & ALGORITHMIC ADVICE
# ==========================================
alerts = []
for _, row in cat_summary.iterrows():
    cat = row['category']
    hist_avg = row['historical_monthly_avg']
    curr_spent = current_totals.get(cat, 0.0)
    
    if curr_spent > hist_avg * 1.10:
        alerts.append(f"⚠️ <b>Overspending Alert:</b> You've spent ${curr_spent:,.0f} on <i>{cat}</i> this month. Your normal average is only ${hist_avg:,.0f}.")

upcoming_bills = []
fixed_expenses = outflows[outflows['class_type'] == 'Fixed Expense']
if not fixed_expenses.empty:
    recent_fixed_logs = fixed_expenses.groupby('category').last().reset_index()
    for _, row in recent_fixed_logs.iterrows():
        upcoming_bills.append(f"📅 <b>Upcoming Bill:</b> Make sure you have ${row['amount']:,.0f} ready for <i>{row['category']}</i>.")

recommendations = []
if predicted_global_30 > avg_monthly_income:
    recommendations.append(f"📉 <b>Forecast Warning:</b> The AI expects you to spend ${predicted_global_30:,.0f} next month, which is higher than your average income (${avg_monthly_income:,.0f}). Time to tighten the belt.")
else:
    recommendations.append(f"📈 <b>On Track:</b> Your projected spending (${predicted_global_30:,.0f}) leaves a healthy margin for savings compared to your income.")

if not cat_summary.empty:
    dominant_drain = cat_summary.loc[cat_summary['amount'].idxmax()]
    recommendations.append(f"💡 <b>Top Tip:</b> <i>{dominant_drain['category']}</i> is your biggest historical expense. Trimming it by just 10% will save you roughly ${dominant_drain['historical_monthly_avg'] * 0.10:,.0f} every single month.")

if not alerts:
    alerts.append("✅ <b>Looking Good:</b> All your spending is currently below your historical averages.")


# ==========================================
# 5. GENERATE AI FORECAST NUMERICAL TABLES 
# ==========================================
forecast_html = """
<div class="w-full flex flex-col">
    <h3 class="text-xl font-bold text-purple-400 mb-6 border-b border-slate-700 pb-3 flex items-center">
        <span class="mr-3 text-2xl">🔮</span> AI Expected Spending (Next 30 Days)
    </h3>
    <div class="grid grid-cols-2 gap-8">
"""

for class_type in ['Fixed Expense', 'Variable Expense']:
    type_data = cat_summary[cat_summary['class_type'] == class_type]
    if not type_data.empty:
        type_data = type_data.sort_values('sub_cat_forecast', ascending=False)
        type_total = type_data['sub_cat_forecast'].sum()
        
        forecast_html += f"""
        <div class="bg-slate-900/40 rounded-xl p-6 border border-slate-700 shadow-sm">
            <div class="flex justify-between items-center mb-4 border-b border-slate-600 pb-2">
                <h4 class="text-lg font-bold text-slate-200">{class_type}</h4>
                <span class="text-lg font-bold text-purple-400">${type_total:,.0f}</span>
            </div>
            <ul class="space-y-3">
        """
        for _, row in type_data.iterrows():
            if row['sub_cat_forecast'] > 0:
                forecast_html += f"""
                <li class="flex justify-between items-center text-slate-300">
                    <span class="text-base">{row['category']}</span>
                    <span class="text-base font-semibold text-slate-100">${row['sub_cat_forecast']:,.0f}</span>
                </li>
                """
        forecast_html += "</ul></div>"

forecast_html += "</div></div>"


# ==========================================
# 6. AI INVESTMENT & WEALTH PLANNING (NEW)
# ==========================================
investment_advice = []
if avg_monthly_savings > 0:
    emergency_fund = avg_monthly_expense * 3
    sip_amount = avg_monthly_savings * 0.50
    fd_amount = avg_monthly_savings * 0.30
    
    investment_advice.append(f"🛡️ <b>Emergency Fund:</b> Build a liquid safety net of <b>${emergency_fund:,.0f}</b> (3x your average monthly expenses) before aggressive investing.")
    investment_advice.append(f"📈 <b>Mutual Funds (SIP):</b> Consider allocating <b>${sip_amount:,.0f}</b> (50% of your monthly savings) into Index or Equity Mutual Funds for long-term wealth compounding.")
    investment_advice.append(f"🏦 <b>Fixed Deposits (FD) / Bonds:</b> Secure <b>${fd_amount:,.0f}</b> (30% of your savings) into risk-free FDs to guarantee stable, predictable returns.")
else:
    investment_advice.append("⚠️ <b>Focus on Cash Flow:</b> Your average monthly savings are currently limited. Prioritize the AI budget cuts above to build a cash surplus before exploring investments.")


# ==========================================
# 7. HIGH-FIDELITY PLOTLY GRAPHICS ENGINE
# ==========================================
# Chart 1: Fixed vs Variable Distribution Structure (FIXED CLIPPING)
fig_dist = px.sunburst(cat_summary, path=['class_type', 'category'], values='amount',
                      title="Where Does Your Money Go?",
                      color='class_type', color_discrete_map={'Fixed Expense':'#3b82f6', 'Variable Expense':'#f59e0b'})
fig_dist.update_layout(
    title=dict(x=0.5, font=dict(size=18)), # Centered the title to prevent side clipping
    paper_bgcolor='rgba(0,0,0,0)', 
    font=dict(color='#f8fafc', size=15), 
    margin=dict(t=60, b=20, l=40, r=40) # Increased left/right margins
)
fig_dist.update_traces(textfont_size=16, textinfo='label+percent entry')
dist_chart_html = fig_dist.to_html(full_html=False, include_plotlyjs='cdn')

# Chart 2: Refined & Spaced Historical vs Current Comparison
fig_comp = go.Figure(data=[
    go.Bar(name='Your Normal Average', x=categories_list, y=historical_averages, marker_color='#64748b'),
    go.Bar(name='Spent This Month', x=categories_list, y=current_month_actuals, marker_color='#06b6d4')
])
fig_comp.update_layout(
    barmode='group',
    title=dict(text="This Month vs. Normal Average", font=dict(size=18, color='white')),
    paper_bgcolor='rgba(0,0,0,0)', plot_bgcolor='rgba(0,0,0,0)',
    font=dict(color='#f8fafc', size=14),
    margin=dict(t=70, b=60, l=50, r=40),
    xaxis=dict(gridcolor='#334155', tickangle=25, tickfont=dict(size=14, color='#cbd5e1')),
    yaxis=dict(gridcolor='#334155', tickprefix="$", tickfont=dict(size=14)),
    legend=dict(orientation="h", yanchor="bottom", y=1.02, xanchor="right", x=1, font=dict(size=14))
)
comp_chart_html = fig_comp.to_html(full_html=False, include_plotlyjs=False)

# ==========================================
# 8. HTML GRAPHICS COMPOSITION & ASSEMBLY
# ==========================================
html_markup = f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Smart Financial Dashboard</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <style>
        body {{ background-color: #0f172a; color: #f8fafc; font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; }}
        .card-container {{ background-color: #1e293b; border: 1px solid #334155; border-radius: 0.85rem; padding: 2rem; box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.3); }}
        .metric-label {{ color: #94a3b8; font-size: 0.85rem; text-transform: uppercase; letter-spacing: 0.05em; font-weight: 600; }}
        .metric-value {{ font-size: 2.1rem; font-weight: 700; color: #ffffff; margin-top: 0.35rem; }}
    </style>
</head>
<body class="p-8">
    <div class="max-w-[1600px] mx-auto space-y-8">
        
        <header class="flex justify-between items-center pb-4 border-b border-slate-700">
            <div>
                <h1 class="text-3xl font-bold text-blue-400">Smart Financial Dashboard</h1>
                <p class="text-slate-400 text-base mt-1">Your AI Assistant for Budgeting and Savings</p>
            </div>
            <div class="text-right text-sm text-emerald-400 font-semibold bg-emerald-900/30 px-4 py-2 rounded-full border border-emerald-800">
                Dashboard Active & Updated
            </div>
        </header>

        <div class="grid grid-cols-3 gap-6">
            <div class="card-container border-l-4 border-l-blue-500">
                <div class="metric-label">Total Earned (All Time)</div>
                <div class="metric-value">${total_income:,.0f}</div>
            </div>
            <div class="card-container border-l-4 border-l-amber-500">
                <div class="metric-label">Total Spent (All Time)</div>
                <div class="metric-value">${total_expense:,.0f}</div>
            </div>
            <div class="card-container border-l-4 border-l-emerald-500">
                <div class="metric-label">Total Savings (All Time)</div>
                <div class="metric-value">${net_savings:,.0f}</div>
            </div>
        </div>

        <div class="grid grid-cols-3 gap-6">
            <div class="card-container border-l-4 border-l-cyan-400">
                <div class="metric-label">Average Monthly Income</div>
                <div class="metric-value">${avg_monthly_income:,.0f}</div>
            </div>
            <div class="card-container border-l-4 border-l-rose-500">
                <div class="metric-label">Average Monthly Spending</div>
                <div class="metric-value">${avg_monthly_expense:,.0f}</div>
            </div>
            <div class="card-container border-l-4 border-l-purple-500">
                <div class="metric-label">Average Monthly Savings</div>
                <div class="metric-value">${avg_monthly_savings:,.0f}</div>
            </div>
        </div>

        <div class="grid grid-cols-3 gap-6">
            <div class="card-container h-[500px] p-4 flex items-center justify-center overflow-hidden col-span-1">{dist_chart_html}</div>
            <div class="card-container h-[500px] p-6 flex items-center justify-center overflow-hidden col-span-2">{comp_chart_html}</div>
        </div>

        <div class="card-container p-8">
            {forecast_html}
        </div>

        <div class="grid grid-cols-2 gap-6">
            <div class="card-container space-y-4">
                <h3 class="text-xl font-bold text-rose-400 flex items-center border-b border-slate-700 pb-3">
                    <span class="mr-3 text-2xl">🚨</span> Alerts & Upcoming Bills
                </h3>
                <ul class="space-y-4 text-slate-200 text-lg leading-relaxed">
                    {''.join([f"<li class='bg-slate-900/60 p-4 rounded-lg border border-slate-700 shadow-sm'>{a}</li>" for a in alerts])}
                    {''.join([f"<li class='bg-amber-900/30 p-4 rounded-lg border border-amber-800/50 text-amber-200 shadow-sm'>{b}</li>" for b in upcoming_bills])}
                </ul>
            </div>
            
            <div class="card-container space-y-4">
                <h3 class="text-xl font-bold text-emerald-400 flex items-center border-b border-slate-700 pb-3">
                    <span class="mr-3 text-2xl">💡</span> AI Advice & Tips
                </h3>
                <ul class="space-y-4 text-slate-200 text-lg leading-relaxed">
                    {''.join([f"<li class='bg-slate-900/60 p-4 rounded-lg border border-slate-700 shadow-sm'>{r}</li>" for r in recommendations])}
                </ul>
            </div>
        </div>
        
        <div class="card-container mt-4 mb-8">
            <h3 class="text-xl font-bold text-indigo-400 flex items-center border-b border-slate-700 pb-3">
                <span class="mr-3 text-2xl">💸</span> AI Investment & Future Wealth Planning
            </h3>
            <ul class="space-y-4 text-slate-200 text-lg leading-relaxed mt-5">
                {''.join([f"<li class='bg-slate-900/60 p-4 rounded-lg border border-slate-700 shadow-sm'>{i}</li>" for i in investment_advice])}
            </ul>
        </div>
        
    </div>
</body>
</html>
"""

# Output compilation artifact and launch browser context safely
target_path = os.path.abspath("interactive_dashboard.html")
with open(target_path, "w", encoding="utf-8") as out_file:
    out_file.write(html_markup)

webbrowser.open('file://' + target_path)