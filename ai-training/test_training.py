import copy
import unittest
from train import FEATURES, LABELS, exported_probabilities, train_dataset

def samples():
    rows=[]
    for label in LABELS:
        for session in range(4):
            for n in range(12):
                features=[0.0]*16
                features[0]=8.0 if label=='IMPACT' else 1.5 if label=='VIBRATION' else 1.0
                features[4]=.25 if label=='VIBRATION' else .02
                features[13]=2000;features[14]=10
                rows.append({'label':label,'group':label+str(session),'ruleLabel':label,'features':features})
    return {'schemaVersion':1,'source':'synthetic-test','deviceId':'test-device','featureNames':FEATURES,'samples':rows}

class TrainingTests(unittest.TestCase):
    def test_synthetic_data_is_not_accepted_as_a_real_model(self):
        with self.assertRaisesRegex(ValueError,'mẫu|thu thật'):
            train_dataset(samples())
    def test_group_isolation_and_export_parity(self):
        model=train_dataset(samples(),require_real=False)
        e=model['evaluation']
        self.assertTrue(set(e['trainGroups']).isdisjoint(e['testGroups']))
        for vector in model['validationVectors']:
            actual=exported_probabilities(model['trees'],vector['features'])
            for a,b in zip(actual,vector['probabilities']):self.assertAlmostEqual(a,b,places=7)
        self.assertEqual(e['source'],'synthetic-test')
    def test_too_few_independent_groups_is_rejected(self):
        data=samples()
        for r in data['samples']:r['group']='one-session'
        with self.assertRaisesRegex(ValueError,'Chưa đủ'):
            train_dataset(data,require_real=False)
    def test_clipped_and_nonfinite_features_are_rejected(self):
        for index,value in [(15,1.0),(0,float('nan'))]:
            data=samples();data['samples'][0]['features'][index]=value
            with self.assertRaises(ValueError):train_dataset(data,require_real=False)

if __name__=='__main__':unittest.main()
