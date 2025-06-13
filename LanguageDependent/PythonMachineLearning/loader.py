import joblib
from utils import preprocess_dataset
import pandas as pd

if __name__ == "__main__":
    
    # Load Dataset
    df = pd.read_csv('./merged.csv')
    print(f"Dataset loaded with {len(df)} records.")

    # Preprocess Dataset
    df = preprocess_dataset(df)
    df.drop(columns=['label'],inplace=True)
    
    print("Dataset preprocessed successfully.")
    print(df.head())

    loaded = joblib.load("test.model")
    
    print(type(loaded))
    
    predicts = loaded.predict(df)
    
    print(predicts)