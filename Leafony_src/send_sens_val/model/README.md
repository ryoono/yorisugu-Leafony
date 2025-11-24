光センサ値(標準化後)からロジスティック回帰モデルを学習する
精度とヘッダーファイルも出力する


```sh
> py .\create_model.py
Loaded: y=(1282,), X_std=(1282, 18)
mean:  [228.67082683 137.15834633 134.01716069 179.26287051 132.57020281
 219.83229329 196.00312012  90.92589704  37.0850234   26.46021841
  64.36193448 137.5374415  176.32527301  70.90327613  49.29017161
  37.66926677  42.40015601  68.46021841]
scale: [ 60.48383372  79.93528416 100.38204564  87.08588841 110.11716984
 109.06033533  97.47125656  90.3870597   52.61642415  27.63440381
  71.26954429 102.64351736  54.24962067  74.27554429  60.2451185
  59.54423734  58.3866538   65.56794044]
Train: X=(897, 18), Test: X=(385, 18)
C:\Users\k1602\AppData\Local\Programs\Python\Python313\Lib\site-packages\sklearn\linear_model\_logistic.py:1247: FutureWarning: 'multi_class' was deprecated in version 1.5 and will be removed in 1.7. From then on, it will always use 'multinomial'. Leave it to its default value to avoid this warning.
  warnings.warn(
====================================
Multiclass Logistic Regression (trained on X_std)
Accuracy: 0.9974
====================================
Classification report:
              precision    recall  f1-score   support

           0     1.0000    0.9896    0.9948        96
           1     1.0000    1.0000    1.0000        95
           2     0.9897    1.0000    0.9948        96
           3     1.0000    1.0000    1.0000        98

    accuracy                         0.9974       385
   macro avg     0.9974    0.9974    0.9974       385
weighted avg     0.9974    0.9974    0.9974       385


Header file written to: logistic_params_raw.h
```