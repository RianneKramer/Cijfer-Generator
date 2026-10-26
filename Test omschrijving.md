# Test omschrijving

## Voeg Tilmann toe als student

```sql
runuser -u postgres -- psql -d Cijfer_Admin_DB -c "
WITH new_student AS (
  INSERT INTO subject_student (student, subject, year, nr_exams)
  VALUES ('Tilmann', 'embedded', 2025, 1)
  RETURNING id
)
INSERT INTO final_result (sub_stu_id, final_result)
SELECT id, NULL FROM new_student;"
```

```sql
INSERT 0 1
```

## Check of Tilmann er echt bijstaat als student zonder exam results

```sql
runuser -u postgres -- psql -d Cijfer_Admin_DB -c "
SELECT s.student, s.subject, s.year, e.exam_nr, e.exam_result, f.final_result
FROM subject_student s
JOIN final_result f ON f.sub_stu_id = s.id
LEFT JOIN exam_result e ON e.final_result_id = f.final_result_id
WHERE s.student = 'Tilmann' AND s.subject = 'embedded' AND s.year = 2025;"
```

```
 student | subject  | year | exam_nr | exam_result | final_result 
---------+----------+------+---------+-------------+--------------
 Tilmann | embedded | 2025 |         |             |             
(1 row)
```

## Vraag exam results aan

```bash
ros2 action send_goal /closing \
  groep4_26_assign1_interface_pkg/action/Students \
  "{year: 2025}" --feedback
```

```
Sending goal:
  year: 2025
  student: []
  subject: []
  nr_exams: []
```

```
Goal accepted with ID: 64d55715598c4b0fa4188a15c0873d1e
```

```
Feedback:
  student:
    - Tilmann
  subject:
    - embedded
  year:
    - 2025
  received_exams:
    - 1
  nr_exams:
    - 1
```

```
Result:
  student:
    - Tilmann
  subject:
    - embedded
  year:
    - 2025
  result:
    - 2.1
```

```
Goal finished with status: SUCCEEDED
```

## Check of ze er nu bij staan

```sql
runuser -u postgres -- psql -d Cijfer_Admin_DB -c "
SELECT s.student, s.subject, s.year, e.exam_nr, e.exam_result, f.final_result
FROM subject_student s
JOIN final_result f ON f.sub_stu_id = s.id
LEFT JOIN exam_result e ON e.final_result_id = f.final_result_id
WHERE s.student = 'Tilmann' AND s.subject = 'embedded' AND s.year = 2025;"
```

```
 student | subject  | year | exam_nr | exam_result | final_result 
---------+----------+------+---------+-------------+--------------
 Tilmann | embedded | 2025 |       1 |    2.071354 |     2.100000
(1 row)
```


