"""Huấn luyện Random Forest và xuất cây JSON cho backend Java. Không nạp mô hình tự động."""
import argparse
from collections import Counter
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import sys
import uuid
import numpy as np
import sklearn
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, confusion_matrix, precision_recall_fscore_support
from sklearn.model_selection import GroupShuffleSplit

LABELS = ('IMPACT', 'NORMAL', 'VIBRATION')
LABEL_VI = {'IMPACT': 'Va đập', 'NORMAL': 'Bình thường', 'VIBRATION': 'Rung lắc', 'UNCERTAIN': 'Chưa chắc chắn'}
FEATURES = ['peak_g','min_g','mean_g','std_g','rms_dynamic_g','gyro_rms_dps','gyro_peak_dps',
            'jerk_peak_gps','above_impact_ms','below_freefall_ms','ax_std','ay_std','az_std',
            'window_ms','sample_period_ms','saturated_ratio']

def metrics(truth, predicted):
    precision, recall, f1, support = precision_recall_fscore_support(truth, predicted, labels=list(LABELS), zero_division=0)
    return {'accuracy': float(accuracy_score(truth, predicted)), 'macroF1': float(np.mean(f1)),
            'coverage': float(np.mean(np.asarray(predicted) != 'UNCERTAIN')),
            'perClass': {label: {'precision': float(precision[i]), 'recall': float(recall[i]),
                                'f1': float(f1[i]), 'support': int(support[i])} for i, label in enumerate(LABELS)},
            'confusion': confusion_matrix(truth, predicted, labels=[*LABELS, 'UNCERTAIN']).tolist()}

def exported_probabilities(trees, vector):
    result = np.zeros(3)
    values = np.asarray(vector, dtype=np.float32)
    for tree in trees:
        index = 0
        while tree[index]['feature'] != -1:
            node = tree[index]
            index = node['left'] if float(values[node['feature']]) <= node['threshold'] else node['right']
        result += np.asarray(tree[index]['probabilities']) / len(trees)
    return result

def train_dataset(dataset, score_threshold=.7, require_real=True):
    if dataset.get('schemaVersion') != 1 or dataset.get('featureNames') != FEATURES:
        raise ValueError('Bộ dữ liệu không đúng phiên bản đặc trưng của dự án.')
    if require_real and dataset.get('source') != 'real-device':
        raise ValueError('Chỉ huấn luyện để sử dụng từ bộ dữ liệu thu thật trên thiết bị.')
    device = dataset.get('deviceId', '')
    if not re.fullmatch(r'[A-Za-z0-9_-]{1,64}', device):
        raise ValueError('Mã thiết bị không hợp lệ.')
    rows = dataset.get('samples', [])
    if not isinstance(rows, list) or len(rows) > 10000:
        raise ValueError('Bộ dữ liệu phải có tối đa 10.000 đoạn.')
    if not .5 <= score_threshold <= .95:
        raise ValueError('Ngưỡng điểm dự đoán phải từ 0,5 đến 0,95.')
    if any(r.get('label') not in LABELS or not isinstance(r.get('group'), str) or not r['group'].strip()
           or r.get('ruleLabel') not in LABELS for r in rows):
        raise ValueError('Mỗi đoạn phải có nhãn quan sát, nhóm buổi thử và kết quả quy tắc hợp lệ.')
    counts = Counter(r['label'] for r in rows)
    groups_per_class = {label: len({r['group'] for r in rows if r['label'] == label}) for label in LABELS}
    missing = [f'{LABEL_VI[label]}: {counts[label]}/30 đoạn, {groups_per_class[label]}/3 nhóm'
               for label in LABELS if counts[label] < 30 or groups_per_class[label] < 3]
    if missing:
        raise ValueError('Chưa đủ dữ liệu thật: ' + '; '.join(missing))
    x = np.asarray([r.get('features', []) for r in rows], dtype=np.float32)
    if x.shape != (len(rows), len(FEATURES)) or not np.isfinite(x).all():
        raise ValueError('Đặc trưng thiếu cột hoặc có giá trị không hữu hạn.')
    if (x[:, 15] != 0).any() or (x[:, 13] < 1750).any() or (x[:, 13] > 2500).any():
        raise ValueError('Bộ dữ liệu chứa đoạn chạm giới hạn hoặc thời lượng không hợp lệ.')
    y = np.asarray([r['label'] for r in rows])
    groups = np.asarray([r['group'] for r in rows])
    split = None
    for train, test in GroupShuffleSplit(n_splits=100, test_size=.34, random_state=42).split(x, y, groups):
        if set(y[train]) == set(LABELS) and set(y[test]) == set(LABELS):
            split = train, test
            break
    if split is None:
        raise ValueError('Không thể tách buổi thử có đủ ba nhãn ở cả hai tập. Thu thêm các nhóm buổi thử độc lập.')
    train, test = split
    assert set(groups[train]).isdisjoint(groups[test])
    forest = RandomForestClassifier(n_estimators=32, max_depth=6, min_samples_leaf=2,
                                    class_weight='balanced', random_state=42, n_jobs=2)
    forest.fit(x[train], y[train])
    probabilities = forest.predict_proba(x[test])
    candidates = forest.classes_[np.argmax(probabilities, axis=1)]
    predicted = np.where(np.max(probabilities, axis=1) >= score_threshold, candidates, 'UNCERTAIN')
    baseline = np.asarray([rows[i]['ruleLabel'] for i in test])
    trees = []
    for estimator in forest.estimators_:
        tree = estimator.tree_
        nodes = []
        for i in range(tree.node_count):
            leaf = tree.children_left[i] == -1
            p = tree.value[i][0].astype(float)
            p = p / p.sum()
            nodes.append({'feature': -1 if leaf else int(tree.feature[i]), 'threshold': float(tree.threshold[i]),
                          'left': int(tree.children_left[i]), 'right': int(tree.children_right[i]),
                          'probabilities': p.tolist() if leaf else []})
        trees.append(nodes)
    checks = []
    for index in test[:10]:
        expected = forest.predict_proba(x[index:index+1])[0]
        actual = exported_probabilities(trees, x[index])
        if not np.allclose(expected, actual, atol=1e-7, rtol=0):
            raise ValueError('Kiểm tra xuất cây không khớp với scikit-learn.')
        checks.append({'features': x[index].astype(float).tolist(), 'probabilities': expected.astype(float).tolist()})
    return {'schemaVersion': 1, 'version': 'rf-' + uuid.uuid4().hex[:16],
            'trainedAt': datetime.now(timezone.utc).isoformat(), 'sklearnVersion': sklearn.__version__, 'deviceId': device,
            'featureNames': FEATURES, 'classes': forest.classes_.tolist(), 'trees': trees,
            'scoreThreshold': score_threshold, 'validationVectors': checks,
            'evaluation': {'source': dataset.get('source'), 'groupSplit': True,
                'trainGroups': sorted(set(groups[train])), 'testGroups': sorted(set(groups[test])),
                'trainCount': len(train), 'testCount': len(test), 'labelCounts': dict(counts),
                'groupsPerClass': groups_per_class, 'ai': metrics(y[test], predicted), 'baseline': metrics(y[test], baseline)}}

def report_text(model):
    e = model['evaluation']
    lines = ['KẾT QUẢ KIỂM TRA TRÊN CÁC BUỔI THỬ CHƯA DÙNG ĐỂ HUẤN LUYỆN',
             f"Huấn luyện: {e['trainCount']} đoạn / {len(e['trainGroups'])} nhóm; kiểm tra: {e['testCount']} đoạn / {len(e['testGroups'])} nhóm.",
             'Các nhóm giữa hai tập không trùng nhau.', '']
    for key, title in [('ai', 'Mô hình AI'), ('baseline', 'Quy tắc hiện tại trên cùng đoạn đo')]:
        m = e[key]
        lines += [title, f"Tỷ lệ đúng: {m['accuracy']:.1%}; F1 trung bình: {m['macroF1']:.1%}; tỷ lệ có kết luận: {m['coverage']:.1%}."]
        for label in LABELS:
            c = m['perClass'][label]
            lines.append(f"  {LABEL_VI[label]}: độ chính xác {c['precision']:.1%}; tỷ lệ phát hiện {c['recall']:.1%}; F1 {c['f1']:.1%}; số mẫu {c['support']}.")
        lines.append('')
    lines += ['Điểm dự đoán không phải xác suất chắc chắn đã được hiệu chuẩn.',
              'Chỉ áp dụng kết quả cho điều kiện đã thử; AI chạy song song và không hủy cảnh báo theo ngưỡng.',
              'Không điều chỉnh mô hình dựa vào tập kiểm tra này rồi dùng lại cùng tập để báo thành tích.']
    return '\n'.join(lines) + '\n'

def main():
    parser = argparse.ArgumentParser(description='Huấn luyện AI phân biệt bình thường, rung lắc và va đập.')
    parser.add_argument('dataset', type=Path, help='Tệp dữ liệu JSON xuất từ dashboard')
    parser.add_argument('--output', type=Path, default=Path('mo-hinh-ai.json'), help='Tệp mô hình JSON đầu ra')
    parser.add_argument('--score-threshold', type=float, default=.7, help='Ngưỡng điểm dự đoán, chọn trước khi kiểm tra')
    args = parser.parse_args()
    try:
        model = train_dataset(json.loads(args.dataset.read_text(encoding='utf-8-sig')), args.score_threshold)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(model, ensure_ascii=False, allow_nan=False, indent=2), encoding='utf-8')
        report = args.output.with_suffix('.bao-cao.txt')
        report.write_text(report_text(model), encoding='utf-8')
        print(report_text(model))
        print('Đã lưu mô hình:', args.output, '\nĐã lưu báo cáo:', report)
    except (ValueError, OSError, KeyError, TypeError) as error:
        print('Không huấn luyện được:', error, file=sys.stderr)
        return 1
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
