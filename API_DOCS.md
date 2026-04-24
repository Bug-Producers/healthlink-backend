# HealthLink API Documentation

## Overview

HealthLink is a medical appointment platform. Doctors manage their schedules and record cash payments. Patients browse doctors, book appointments (auto-assigned times), and leave ratings. All endpoints require a Firebase ID token in the `Authorization: Bearer <token>` header.

---

## Authentication

Every request must include a valid Firebase ID token:

```
Authorization: Bearer <firebase_id_token>
```

The token's `sub` (subject) claim is used as the user ID for all operations.

---

## Doctor Endpoints

### `GET /api/doctors/profile`
Returns the authenticated doctor's profile.

**Response:** `200` with doctor JSON | `401` if unauthorized

---

### `PUT /api/doctors/profile`
Updates the doctor's profile fields.

**Body:**
```json
{
  "name": "Dr. Smith",
  "city": "Cairo",
  "about": "Cardiologist with 10 years exp",
  "appointmentDuration": 30,
  "bufferTime": 10
}
```

**Response:** `200` on success | `400` invalid body

---

### `PUT /api/doctors/profile/image`
Uploads a profile image as base64.

**Body:**
```json
{
  "image": "<base64_encoded_image_data>"
}
```

**Response:** `200` on success

---

### `GET /api/doctors/schedule`
Returns the doctor's weekly availability.

**Response:**
```json
{
  "doctorId": "abc123",
  "availability": {
    "monday": [{"startTime": "09:00", "endTime": "14:00"}],
    "wednesday": [{"startTime": "10:00", "endTime": "16:00"}]
  }
}
```

---

### `PUT /api/doctors/schedule`
Replaces the doctor's full weekly schedule.

**Body:**
```json
{
  "availability": {
    "monday": [
      {"startTime": "09:00", "endTime": "14:00"},
      {"startTime": "16:00", "endTime": "19:00"}
    ],
    "tuesday": [
      {"startTime": "10:00", "endTime": "15:00"}
    ]
  }
}
```

---

### `GET /api/doctors/appointments`
Lists all appointments for the doctor. The returned objects include inline patient details (`patientName`, `patientImage`) and `status`.

---

### `PATCH /api/doctors/appointments/<id>`
Updates an appointment's status (e.g., Booked=0, Completed=1, Cancelled=2).

**Body:**
```json
{
  "status": 1
}
```

---

### `POST /api/doctors/payments`
Records a cash payment from a patient.

**Body:**
```json
{
  "patientId": "patient_uid",
  "appointmentId": "apt_123",
  "amount": 40.0
}
```

---

### `GET /api/doctors/payments`
Lists all payment records for the doctor.

---

### `GET /api/doctors/revenue`
Returns total earnings, payment count, and an array of daily totals (`dailyBreakdown`) for chart generation.

**Response:**
```json
{
  "totalEarnings": 1200.0,
  "totalPayments": 30,
  "dailyBreakdown": [
    {"date": "2025-04-20", "total": 120.0}
  ]
}
```

---

### `GET /api/doctors/ratings`
Shows all ratings and the average score.

---

### `GET /api/doctors/notifications`
View dynamic system push notifications triggered proactively during patient bookings or cancellations.

---

### `PATCH /api/doctors/notifications/<id>`
Marks a specific notification as read.

---

## Patient Endpoints

### `GET /api/patients/<id>`
Fetches the patient profile including Name and Profile Photo.

---

### `GET /api/patients/doctors`
Browse all available doctors.

---

### `GET /api/patients/doctors/<doctorId>`
View a specific doctor's full profile with live average rating.

---

### `GET /api/patients/departments/<name>`
Browse doctors filtered by department name.

---

### `GET /api/patients/doctors/<doctorId>/slots/<day>`
View available time slots for a doctor on a specific day (e.g., `monday`).

---

### `POST /api/patients/appointments/book`
Book an appointment. The system securely scans MongoDB records globally to prevent double-booking overlaps and dynamically offsets the exact arrival time by the respective Doctor's buffer duration.

**Body:**
```json
{
  "doctorId": "doctor_uid",
  "date": "2025-04-20",
  "dayOfWeek": "monday",
  "frameStart": "09:00",
  "frameEnd": "14:00"
}
```

**Response:**
```json
{
  "id": "apt_17...",
  "startTime": "09:30",
  "endTime": "10:00",
  "message": "Booked! Your appointment is at 09:30"
}
```

---

### `GET /api/patients/appointments`
Lists all the patient's appointments.

---

### `DELETE /api/patients/appointments/<id>`
Cancels an appointment (marks it, doesn't delete).

---

### `POST /api/patients/ratings`
Rate a doctor after a visit.

**Body:**
```json
{
  "doctorId": "doctor_uid",
  "stars": 5,
  "comment": "Great experience!"
}
```

---

### `GET /api/patients/history`
View medical history (newest report first — LIFO stack).

---

### `POST /api/patients/history`
Allows a doctor (or system) to append a diagnosis or prescription report to a patient's history.

**Body:**
```json
{
  "patientId": "patient_uid",
  "report": "Patient presented with mild fever. Prescribed Paracetamol."
}
```

---

### `GET /api/patients/notifications`
View dynamic push notifications from doctor actions and appointment booking confirmations.

---

### `POST /api/dev/seed`
**(Development Only)** Instantly generates 30 fully structured mock entities directly into MongoDB (Patients & Doctors), complete with weekly schedules, ratings, and LIFO medical histories.

---

## Data Structures Used

| Structure | Usage |
|-----------|-------|
| **LinkedList** | Doctors per department, time slot lists |
| **Queue (FIFO)** | Patient bookings per time slot — first-come-first-served |
| **Stack (LIFO)** | Patient medical history — newest report on top |
| **Unordered Map** | Doctor availability (day → slots), in-memory caches |
| **Tree** | API endpoint organization (`/api/doctors/profile`) |
