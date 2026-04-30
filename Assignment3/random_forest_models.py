"""
Random Forest Models for Throughput and Response Time Prediction
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import warnings
warnings.filterwarnings('ignore')

from sklearn.ensemble import RandomForestRegressor
from sklearn.model_selection import RandomizedSearchCV
from sklearn.metrics import mean_absolute_percentage_error, mean_squared_error, r2_score
from scipy.stats import randint as sp_randint

# ============================================
# DATA LOADING AND PREPROCESSING
# ============================================

print("="*60)
print("RANDOM FOREST MODELS FOR THROUGHPUT AND RESPONSE TIME")
print("="*60)

# Load the data
df_train = pd.read_csv('ram.csv')
df_test = pd.read_csv('testram.csv')

print("\nDataset shapes:")
print(f"  Training: {df_train.shape}")
print(f"  Test: {df_test.shape}")

# Data preprocessing
input_features = ['ArrivalRate', 'device_write', 'cpus', 'memory', 'size', 'cpuLoad']
output_targets = ['Throughput', 'ResponseTime']

# Helper function for memory conversion
def convert_memory(mem_str):
    if 'g' in mem_str.lower():
        return float(mem_str.lower().replace('g', '')) * 1024
    elif 'm' in mem_str.lower():
        return float(mem_str.lower().replace('m', ''))
    else:
        return float(mem_str)

# Process training data
print("\n" + "="*60)
print("PROCESSING DATA")
print("="*60)

data_train = df_train[input_features + output_targets].copy()
data_train = data_train.dropna(subset=output_targets)
data_train['device_write_numeric'] = data_train['device_write'].str.replace('mb', '').astype(float)
data_train['memory_numeric'] = data_train['memory'].apply(convert_memory)
data_train = data_train[data_train['Throughput'] != 0]

# Process test data
data_test = df_test[input_features + output_targets].copy()
data_test = data_test.dropna(subset=output_targets)
data_test['device_write_numeric'] = data_test['device_write'].str.replace('mb', '').astype(float)
data_test['memory_numeric'] = data_test['memory'].apply(convert_memory)
data_test = data_test[data_test['Throughput'] != 0]

# Prepare feature sets
X_columns = ['ArrivalRate', 'cpus', 'size', 'cpuLoad', 'device_write_numeric', 'memory_numeric']
X_train = data_train[X_columns].values
y_throughput_train = data_train['Throughput'].values
y_responsetime_train = data_train['ResponseTime'].values

X_test = data_test[X_columns].values
y_throughput_test = data_test['Throughput'].values
y_responsetime_test = data_test['ResponseTime'].values

print(f"\nTraining set size: {X_train.shape[0]}")
print(f"Test set size: {X_test.shape[0]}")
print(f"Number of features: {X_train.shape[1]}")

# ============================================
# HYPERPARAMETER TUNING
# ============================================

print("\n" + "="*60)
print("HYPERPARAMETER TUNING")
print("="*60)

# Parameter grid for Random Forest
param_dist = {
    'n_estimators': sp_randint(50, 300),
    'max_depth': [10, 20, 30, 40, None],
    'min_samples_split': sp_randint(2, 20),
    'min_samples_leaf': sp_randint(1, 10),
    'max_features': ['sqrt', 'log2'],
    'bootstrap': [True, False],
}

# Tune Throughput Model
print("\n1. TUNING THROUGHPUT MODEL...")
print("-" * 60)

base_rf_throughput = RandomForestRegressor(random_state=42, n_jobs=-1)
grid_search_rf_throughput = RandomizedSearchCV(
    base_rf_throughput,
    param_dist,
    n_iter=15,
    cv=3,
    n_jobs=-1,
    random_state=42,
    verbose=1,
    scoring='r2'
)

print("Starting RandomizedSearchCV for Throughput...")
grid_search_rf_throughput.fit(X_train, y_throughput_train)
print(f"\nBest parameters: {grid_search_rf_throughput.best_params_}")
print(f"Best CV R² Score: {grid_search_rf_throughput.best_score_:.4f}")

best_params_rf_throughput = grid_search_rf_throughput.best_params_

# Tune Response Time Model
print("\n2. TUNING RESPONSE TIME MODEL...")
print("-" * 60)

base_rf_responsetime = RandomForestRegressor(random_state=42, n_jobs=-1)
grid_search_rf_responsetime = RandomizedSearchCV(
    base_rf_responsetime,
    param_dist,
    n_iter=15,
    cv=3,
    n_jobs=-1,
    random_state=42,
    verbose=1,
    scoring='r2'
)

print("Starting RandomizedSearchCV for Response Time...")
grid_search_rf_responsetime.fit(X_train, y_responsetime_train)
print(f"\nBest parameters: {grid_search_rf_responsetime.best_params_}")
print(f"Best CV R² Score: {grid_search_rf_responsetime.best_score_:.4f}")

best_params_rf_responsetime = grid_search_rf_responsetime.best_params_

# ============================================
# MODEL 1: THROUGHPUT
# ============================================

print("\n" + "="*60)
print("MODEL 1: THROUGHPUT PREDICTION")
print("="*60)

model_rf_throughput = RandomForestRegressor(
    **best_params_rf_throughput,
    random_state=42,
    n_jobs=-1
)

print("\nTraining Random Forest for Throughput...")
model_rf_throughput.fit(X_train, y_throughput_train)
print("Training completed!")

# Evaluate
print("\nEvaluating on Test Data:")
print("-" * 60)

y_throughput_pred = model_rf_throughput.predict(X_test)

mape_throughput = mean_absolute_percentage_error(y_throughput_test, y_throughput_pred)
smape_throughput = np.mean(2 * np.abs(y_throughput_pred - y_throughput_test) / 
                            (np.abs(y_throughput_test) + np.abs(y_throughput_pred)))
rmse_throughput = np.sqrt(mean_squared_error(y_throughput_test, y_throughput_pred))
mae_throughput = np.mean(np.abs(y_throughput_test - y_throughput_pred))
r2_throughput = r2_score(y_throughput_test, y_throughput_pred)

percentage_errors_throughput = np.abs((y_throughput_test - y_throughput_pred) / y_throughput_test) * 100
accuracy_90 = (percentage_errors_throughput <= 90).sum() / len(percentage_errors_throughput) * 100
accuracy_50 = (percentage_errors_throughput <= 50).sum() / len(percentage_errors_throughput) * 100
accuracy_20 = (percentage_errors_throughput <= 20).sum() / len(percentage_errors_throughput) * 100
accuracy_10 = (percentage_errors_throughput <= 10).sum() / len(percentage_errors_throughput) * 100

print(f"MAPE (Mean Absolute Percentage Error): {mape_throughput:.4f}")
print(f"SMAPE (Symmetric MAPE):                {smape_throughput:.4f}")
print(f"RMSE (Root Mean Squared Error):        {rmse_throughput:.4f}")
print(f"MAE (Mean Absolute Error):             {mae_throughput:.4f}")
print(f"R² Score:                              {r2_throughput:.4f}")
print(f"\nAccuracy Metrics:")
print(f"  Predictions within 90% error:        {accuracy_90:.2f}%")
print(f"  Predictions within 50% error:        {accuracy_50:.2f}%")
print(f"  Predictions within 20% error:        {accuracy_20:.2f}%")
print(f"  Predictions within 10% error:        {accuracy_10:.2f}%")

print("\nFeature Importances (Top 10):")
feature_importance = pd.DataFrame({
    'Feature': X_columns,
    'Importance': model_rf_throughput.feature_importances_
}).sort_values('Importance', ascending=False)
print(feature_importance.to_string(index=False))

# ============================================
# MODEL 2: RESPONSE TIME
# ============================================

print("\n" + "="*60)
print("MODEL 2: RESPONSE TIME PREDICTION")
print("="*60)

model_rf_responsetime = RandomForestRegressor(
    **best_params_rf_responsetime,
    random_state=42,
    n_jobs=-1
)

print("\nTraining Random Forest for Response Time...")
model_rf_responsetime.fit(X_train, y_responsetime_train)
print("Training completed!")

# Evaluate
print("\nEvaluating on Test Data:")
print("-" * 60)

y_responsetime_pred = model_rf_responsetime.predict(X_test)

mape_responsetime = mean_absolute_percentage_error(y_responsetime_test, y_responsetime_pred)
smape_responsetime = np.mean(2 * np.abs(y_responsetime_pred - y_responsetime_test) / 
                              (np.abs(y_responsetime_test) + np.abs(y_responsetime_pred)))
rmse_responsetime = np.sqrt(mean_squared_error(y_responsetime_test, y_responsetime_pred))
mae_responsetime = np.mean(np.abs(y_responsetime_test - y_responsetime_pred))
r2_responsetime = r2_score(y_responsetime_test, y_responsetime_pred)

percentage_errors_responsetime = np.abs((y_responsetime_test - y_responsetime_pred) / y_responsetime_test) * 100
accuracy_90_rt = (percentage_errors_responsetime <= 90).sum() / len(percentage_errors_responsetime) * 100
accuracy_50_rt = (percentage_errors_responsetime <= 50).sum() / len(percentage_errors_responsetime) * 100
accuracy_20_rt = (percentage_errors_responsetime <= 20).sum() / len(percentage_errors_responsetime) * 100
accuracy_10_rt = (percentage_errors_responsetime <= 10).sum() / len(percentage_errors_responsetime) * 100

print(f"MAPE (Mean Absolute Percentage Error): {mape_responsetime:.4f}")
print(f"SMAPE (Symmetric MAPE):                {smape_responsetime:.4f}")
print(f"RMSE (Root Mean Squared Error):        {rmse_responsetime:.4f}")
print(f"MAE (Mean Absolute Error):             {mae_responsetime:.4f}")
print(f"R² Score:                              {r2_responsetime:.4f}")
print(f"\nAccuracy Metrics:")
print(f"  Predictions within 90% error:        {accuracy_90_rt:.2f}%")
print(f"  Predictions within 50% error:        {accuracy_50_rt:.2f}%")
print(f"  Predictions within 20% error:        {accuracy_20_rt:.2f}%")
print(f"  Predictions within 10% error:        {accuracy_10_rt:.2f}%")

print("\nFeature Importances (Top 10):")
feature_importance = pd.DataFrame({
    'Feature': X_columns,
    'Importance': model_rf_responsetime.feature_importances_
}).sort_values('Importance', ascending=False)
print(feature_importance.to_string(index=False))

# ============================================
# SUMMARY COMPARISON
# ============================================

print("\n" + "="*60)
print("RANDOM FOREST MODELS SUMMARY")
print("="*60)

summary_df = pd.DataFrame({
    'Metric': ['MAPE', 'SMAPE', 'RMSE', 'MAE', 'R²', 'Accuracy (10%)', 'Accuracy (20%)'],
    'Throughput': [
        f"{mape_throughput:.4f}",
        f"{smape_throughput:.4f}",
        f"{rmse_throughput:.4f}",
        f"{mae_throughput:.4f}",
        f"{r2_throughput:.4f}",
        f"{accuracy_10:.2f}%",
        f"{accuracy_20:.2f}%"
    ],
    'Response Time': [
        f"{mape_responsetime:.4f}",
        f"{smape_responsetime:.4f}",
        f"{rmse_responsetime:.4f}",
        f"{mae_responsetime:.4f}",
        f"{r2_responsetime:.4f}",
        f"{accuracy_10_rt:.2f}%",
        f"{accuracy_20_rt:.2f}%"
    ]
})

print("\n" + summary_df.to_string(index=False))
print("\n" + "="*60)
print("Analysis Complete!")
print("="*60)
