
## 1. ConditionalGraph

```mermaid
flowchart TD
    Entry["**Block Entry** [0–14]
    ─────────────────────────────
    line 0, live 2:   %v0 = add 1, 2
    line 1, live 4:   %v1 = add %v0, 10
    line 2, live 6:   %v2 = add %v1, 20
    line 3, live 8:   %v3 = add %v2, 30
    line 4, live 10:  %v4 = icmp sgt %v0, 5
    line 5, live 12:  br %v4, label %IfTrue, label %IfFalse"]

    IfFalse["**Block IfFalse** [14–24]
    ─────────────────────────────
    line 6, live 16:  %v8 = sub %v0, %v1
    line 7, live 18:  %v9 = sub %v2, %v3
    line 8, live 20:  %v10 = add %v8, %v9
    line 9, live 22:  br label %End"]

    IfTrue["**Block IfTrue** [24–34]
    ─────────────────────────────
    line 10, live 26: %v5 = add %v0, %v1
    line 11, live 28: %v6 = add %v2, %v3
    line 12, live 30: %v7 = add %v5, %v6
    line 13, live 32: br label %End"]

    End["**Block End** [34–40]
    ─────────────────────────────
    line 14, live 34: %v11 = phi [ %v7, %IfTrue ], [ %v10, %IfFalse ]
    line 15, live 36: %v12 = add %v11, %v3
    line 16, live 38: ret i32 %v12"]

    Entry -->|"false"| IfFalse
    Entry -->|"true"| IfTrue
    IfFalse --> End
    IfTrue  --> End
```

```mermaid
flowchart TD
    Entry["**Block Entry**
    ─────────────────────────────
    %v0 = add 1, 2
    spill %v0 -> [S3]
    %v14 = fill [S3]
    %v1 = add %v14, 10
    spill %v1 -> [S2]
    %v16 = fill [S2]
    %v2 = add %v16, 20
    spill %v2 -> [S0]
    %v18 = fill [S0]
    %v3 = add %v18, 30
    spill %v3 -> [S1]
    %v20 = fill [S3]
    %v4 [R1] = icmp sgt %v20, 5
    br %v4 [R1], label %IfTrue, label %IfFalse"]

    IfFalse["**Block IfFalse**
    ─────────────────────────────
    %v25 = fill [S3]
    %v26 = fill [S2]
    %v8 [R1] = sub %v25, %v26
    %v27 = fill [S0]
    %v28 = fill [S1]
    %v9 [R0] = sub %v27, %v28
    %v10 [R0] = add %v8 [R1], %v9 [R0]
    br label %End"]

    IfTrue["**Block IfTrue**
    ─────────────────────────────
    %v21 = fill [S3]
    %v22 = fill [S2]
    %v5 [R0] = add %v21, %v22
    %v23 = fill [S0]
    %v24 = fill [S1]
    %v6 [R1] = add %v23, %v24
    %v7 [R0] = add %v5 [R0], %v6 [R1]
    br label %End"]

    End["**Block End**
    ─────────────────────────────
    %v11 [R0] = phi [ %v7 [R0], %IfTrue ], [ %v10 [R0], %IfFalse ]
    %v29 = fill [S1]
    %v12 [R0] = add %v11 [R0], %v29
    ret i32 %v12 [R0]"]

    Entry -->|"false"| IfFalse
    Entry -->|"true"| IfTrue
    IfFalse --> End
    IfTrue  --> End
```

```
=== Liveness Intervals ===
%v0 : [2, 16) [24, 26) 
%v1 : [4, 16) [24, 26) 
%v2 : [6, 18) [24, 28) 
%v3 : [8, 36) 
%v4 : [10, 12) 
%v5 : [26, 30) 
%v6 : [28, 30) 
%v7 : [30, 34) 
%v8 : [16, 20) 
%v9 : [18, 20)
%v10 : [20, 24) 
%v11 : [34, 36) 
%v12 : [36, 38) 
```

### Table 1 — ConditionalGraph

| Instr | %v0 | %v1 | %v2 | %v3 | %v4 | %v5 | %v6 | %v7 | %v8 | %v9 | %v10 | %v11 | %v12 |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| 2 | S3 |  |  |  |  |  |  |  |  |  |  |  |  |
| 4 | S3 | S2 |  |  |  |  |  |  |  |  |  |  |  |
| 6 | S3 | S2 | S0 |  |  |  |  |  |  |  |  |  |  |
| 8 | S3 | S2 | S0 | S1 |  |  |  |  |  |  |  |  |  |
| 10 | S3 | S2 | S0 | S1 | R1 |  |  |  |  |  |  |  |  |
| 12 | S3 | S2 | S0 | S1 |  |  |  |  |  |  |  |  |  |
| 14 | S3 | S2 | S0 | S1 |  |  |  |  |  |  |  |  |  |
| 16 |  |  | S0 | S1 |  |  |  |  | R1 |  |  |  |  |
| 18 |  |  |  | S1 |  |  |  |  | R1 | R0 |  |  |  |
| 20 |  |  |  | S1 |  |  |  |  |  |  | R0 |  |  |
| 22 |  |  |  | S1 |  |  |  |  |  |  | R0 |  |  |
| 24 | S3 | S2 | S0 | S1 |  |  |  |  |  |  |  |  |  |
| 26 |  |  | S0 | S1 |  | R0 |  |  |  |  |  |  |  |
| 28 |  |  |  | S1 |  | R0 | R1 |  |  |  |  |  |  |
| 30 |  |  |  | S1 |  |  |  | R0 |  |  |  |  |  |
| 32 |  |  |  | S1 |  |  |  | R0 |  |  |  |  |  |
| 34 |  |  |  | S1 |  |  |  |  |  |  |  | R0 |  |
| 36 |  |  |  |  |  |  |  |  |  |  |  |  | R0 |


---

## 2. SimpleReducibleLoopGraph

```mermaid
flowchart TD
    Entry["**Block Entry** [0–12]
    ─────────────────────────────
    line 0, live 2:   %v0 = add 0, 0
    line 1, live 4:   %v1 = add 10, 10
    line 2, live 6:   %v2 = add 20, 20
    line 3, live 8:   %v3 = add 30, 30
    line 4, live 10:  br label %LoopHdr"]

    LoopHdr["**Block LoopHdr** [12–18]
    ─────────────────────────────
    line 5, live 12:  %v4 = phi [ %v0, %Entry ], [ %v12, %LoopBdy ]
    line 6, live 12:  %v5 = phi [ %v1, %Entry ], [ %v9, %LoopBdy ]
    line 7, live 12:  %v6 = phi [ %v2, %Entry ], [ %v10, %LoopBdy ]
    line 8, live 12:  %v7 = phi [ %v3, %Entry ], [ %v11, %LoopBdy ]
    line 9, live 14:  %v8 = icmp slt %v4, 10
    line 10, live 16: br %v8, label %LoopBdy, label %Exit"]

    LoopBdy["**Block LoopBdy** [18–30]
    ─────────────────────────────
    line 11, live 20: %v9 = add %v5, 1
    line 12, live 22: %v10 = add %v6, 2
    line 13, live 24: %v11 = add %v7, 3
    line 14, live 26: %v12 = add %v4, 1
    line 15, live 28: br label %LoopHdr"]

    Exit["**Block Exit** [30–38]
    ─────────────────────────────
    line 16, live 32: %v13 = add %v5, %v6
    line 17, live 34: %v14 = add %v13, %v7
    line 18, live 36: ret i32 %v14"]

    Entry  --> LoopHdr
    LoopHdr -->|"true (slt)"| LoopBdy
    LoopHdr -->|"false"| Exit
    LoopBdy --> LoopHdr

```

```mermaid
flowchart TD
    Entry["**Block Entry**
    ─────────────────────────────
    %v0 [R0] = add 0, 0
    %v1 [R1] = add 10, 10
    %v2 = add 20, 20
    spill %v2 -> [S0]
    %v3 = add 30, 30
    spill %v3 -> [S1]
    %v28 = fill [S0]
    %v30 = fill [S1]
    br label %LoopHdr"]

    LoopHdr["**Block LoopHdr**
    ─────────────────────────────
    %v4 [R0] = phi [ %v0 [R0], %Entry ], [ %v12 [R0], %LoopBdy ]
    %v5 = phi [ %v1 [R1], %Entry ], [ %v9 [R1], %LoopBdy ]
    %v6 = phi [ %v28, %Entry ], [ %v29, %LoopBdy ]
    %v7 = phi [ %v30, %Entry ], [ %v31, %LoopBdy ]
    spill %v5 -> [S3]
    spill %v6 -> [S4]
    spill %v7 -> [S2]
    %v8 [R1] = icmp slt %v4 [R0], 10
    br %v8 [R1], label %LoopBdy, label %Exit"]

    LoopBdy["**Block LoopBdy**
    ─────────────────────────────
    %v20 = fill [S3]
    %v9 [R1] = add %v20, 1
    %v21 = fill [S4]
    %v10 = add %v21, 2
    spill %v10 -> [S5]
    %v23 = fill [S2]
    %v11 = add %v23, 3
    spill %v11 -> [S6]
    %v12 [R0] = add %v4 [R0], 1
    %v29 = fill [S5]
    %v31 = fill [S6]
    br label %LoopHdr"]

    Exit["**Block Exit**
    ─────────────────────────────
    %v25 = fill [S3]
    %v26 = fill [S4]
    %v13 [R0] = add %v25, %v26
    %v27 = fill [S2]
    %v14 [R0] = add %v13 [R0], %v27
    ret i32 %v14 [R0]"]

    Entry  --> LoopHdr
    LoopHdr -->|"true (slt)"| LoopBdy
    LoopHdr -->|"false"| Exit
    LoopBdy --> LoopHdr
```

```
=== Liveness Intervals ===
%v0 : [2, 12) 
%v1 : [4, 12) 
%v2 : [6, 12) 
%v3 : [8, 12) 
%v4 : [12, 26) 
%v5 : [12, 20) [30, 32) 
%v6 : [12, 22) [30, 32) 
%v7 : [12, 24) [30, 34) 
%v8 : [14, 16) 
%v9 : [20, 30)
%v10 : [22, 30) 
%v11 : [24, 30) 
%v12 : [26, 30) 
%v13 : [32, 34) 
%v14 : [34, 36) 
```

### Table 2 — SimpleReducibleLoopGraph

| Instr | %v0 | %v1 | %v2 | %v3 | %v4 | %v5 | %v6 | %v7 | %v8 | %v9 | %v10 | %v11 | %v12 | %v13 | %v14 |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| 2 | R0 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 4 | R0 | R1 |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 6 | R0 | R1 | S0 |  |  |  |  |  |  |  |  |  |  |  |  |
| 8 | R0 | R1 | S0 | S1 |  |  |  |  |  |  |  |  |  |  |  |
| 10 | R0 | R1 | S0 | S1 |  |  |  |  |  |  |  |  |  |  |  |
| 12 |  |  |  |  | R0 | S3 | S4 | S2 |  |  |  |  |  |  |  |
| 14 |  |  |  |  | R0 | S3 | S4 | S2 | R1 |  |  |  |  |  |  |
| 16 |  |  |  |  | R0 | S3 | S4 | S2 |  |  |  |  |  |  |  |
| 18 |  |  |  |  | R0 | S3 | S4 | S2 |  |  |  |  |  |  |  |
| 20 |  |  |  |  | R0 |  | S4 | S2 |  | R1 |  |  |  |  |  |
| 22 |  |  |  |  | R0 |  |  | S2 |  | R1 | S5 |  |  |  |  |
| 24 |  |  |  |  | R0 |  |  |  |  | R1 | S5 | S6 |  |  |  |
| 26 |  |  |  |  |  |  |  |  |  | R1 | S5 | S6 | R0 |  |  |
| 28 |  |  |  |  |  |  |  |  |  | R1 | S5 | S6 | R0 |  |  |
| 30 |  |  |  |  |  | S3 | S4 | S2 |  |  |  |  |  |  |  |
| 32 |  |  |  |  |  |  |  | S2 |  |  |  |  |  | R0 |  |
| 34 |  |  |  |  |  |  |  |  |  |  |  |  |  |  | R0 |


---

## 3. NestedConditionInsideLoopGraph

```mermaid
flowchart TD
    Entry["**Block Entry** [0–12]
    ─────────────────────────────
    line 0, live 2:   %v0 = add 0, 0
    line 1, live 4:   %v1 = add 0, 0
    line 2, live 6:   %v2 = add 0, 0
    line 3, live 8:   %v3 = add 0, 0
    line 4, live 10:  br label %Loop"]

    Loop["**Block Loop** [12–18]
    ─────────────────────────────
    line 5, live 12:  %v4 = phi [ %v0, %Entry ], [ %v16, %LoopEnd ]
    line 6, live 12:  %v5 = phi [ %v1, %Entry ], [ %v17, %LoopEnd ]
    line 7, live 12:  %v6 = phi [ %v2, %Entry ], [ %v18, %LoopEnd ]
    line 8, live 12:  %v7 = phi [ %v3, %Entry ], [ %v19, %LoopEnd ]
    line 9, live 14:  %v8 = icmp slt %v7, 10
    line 10, live 16: br %v8, label %If, label %Exit"]

    If["**Block If** [18–24]
    ─────────────────────────────
    line 11, live 20: %v9 = icmp slt %v7, 5
    line 12, live 22: br %v9, label %Then, label %Else"]

    Else["**Block Else** [24–34]
    ─────────────────────────────
    line 13, live 26: %v13 = sub %v4, 1
    line 14, live 28: %v14 = sub %v5, 2
    line 15, live 30: %v15 = sub %v6, 3
    line 16, live 32: br label %LoopEnd"]

    Then["**Block Then** [34–44]
    ─────────────────────────────
    line 17, live 36: %v10 = add %v4, 1
    line 18, live 38: %v11 = add %v5, 2
    line 19, live 40: %v12 = add %v6, 3
    line 20, live 42: br label %LoopEnd"]

    LoopEnd["**Block LoopEnd** [44–50]
    ─────────────────────────────
    line 21, live 44: %v16 = phi [ %v10, %Then ], [ %v13, %Else ]
    line 22, live 44: %v17 = phi [ %v11, %Then ], [ %v14, %Else ]
    line 23, live 44: %v18 = phi [ %v12, %Then ], [ %v15, %Else ]
    line 24, live 46: %v19 = add %v7, 1
    line 25, live 48: br label %Loop"]

    Exit["**Block Exit** [50–58]
    ─────────────────────────────
    line 26, live 52: %v20 = add %v4, %v5
    line 27, live 54: %v21 = add %v20, %v6
    line 28, live 56: ret i32 %v21"]

    Entry  --> Loop
    Loop   -->|"true (slt 10)"| If
    Loop   -->|"false"| Exit
    If     -->|"true (slt 5)"| Then
    If     -->|"false"| Else
    Then   --> LoopEnd
    Else   --> LoopEnd
    LoopEnd --> Loop
```

```mermaid
flowchart TD
    Entry["**Block Entry**
    ─────────────────────────────
    %v0 [R0] = add 0, 0
    %v1 [R1] = add 0, 0
    %v2 [R2] = add 0, 0
    %v3 = add 0, 0
    spill %v3 -> [S0]
    %v40 = fill [S0]
    br label %Loop"]

    Loop["**Block Loop**
    ─────────────────────────────
    %v4 = phi [ %v0 [R0], %Entry ], [ %v16 [R0], %LoopEnd ]
    %v5 = phi [ %v1 [R1], %Entry ], [ %v17 [R2], %LoopEnd ]
    %v6 = phi [ %v2 [R2], %Entry ], [ %v18 [R1], %LoopEnd ]
    %v7 = phi [ %v40, %Entry ], [ %v41, %LoopEnd ]
    %v27 = fill [S4]
    spill %v4 -> [S2]
    spill %v5 -> [S3]
    spill %v6 -> [S1]
    spill %v7 -> [S4]
    %v8 [R2] = icmp slt %v27, 10
    br %v8 [R2], label %If, label %Exit"]

    If["**Block If**
    ─────────────────────────────
    %v28 = fill [S4]
    %v9 [R2] = icmp slt %v28, 5
    br %v9 [R2], label %Then, label %Else"]

    Else["**Block Else**
    ─────────────────────────────
    %v32 = fill [S2]
    %v13 [R2] = sub %v32, 1
    %v33 = fill [S3]
    %v14 [R0] = sub %v33, 2
    %v34 = fill [S1]
    %v15 [R1] = sub %v34, 3
    br label %LoopEnd"]

    Then["**Block Then**
    ─────────────────────────────
    %v29 = fill [S2]
    %v10 [R0] = add %v29, 1
    %v30 = fill [S3]
    %v11 [R1] = add %v30, 2
    %v31 = fill [S1]
    %v12 [R2] = add %v31, 3
    br label %LoopEnd"]

    LoopEnd["**Block LoopEnd**
    ─────────────────────────────
    %v16 [R0] = phi [ %v10 [R0], %Then ], [ %v13 [R2], %Else ]
    %v17 [R2] = phi [ %v11 [R1], %Then ], [ %v14 [R0], %Else ]
    %v18 [R1] = phi [ %v12 [R2], %Then ], [ %v15 [R1], %Else ]
    %v35 = fill [S4]
    %v19 = add %v35, 1
    spill %v19 -> [S5]
    %v41 = fill [S5]
    br label %Loop"]

    Exit["**Block Exit**
    ─────────────────────────────
    %v37 = fill [S2]
    %v38 = fill [S3]
    %v20 [R0] = add %v37, %v38
    %v39 = fill [S1]
    %v21 [R0] = add %v20 [R0], %v39
    ret i32 %v21 [R0]"]

    Entry  --> Loop
    Loop   -->|"true (slt 10)"| If
    Loop   -->|"false"| Exit
    If     -->|"true (slt 5)"| Then
    If     -->|"false"| Else
    Then   --> LoopEnd
    Else   --> LoopEnd
    LoopEnd --> Loop
```

```
=== Liveness Intervals ===
%v0 : [2, 12) 
%v1 : [4, 12) 
%v2 : [6, 12) 
%v3 : [8, 12) 
%v4 : [12, 26) [34, 36) [50, 52) 
%v5 : [12, 28) [34, 38) [50, 52) 
%v6 : [12, 30) [34, 40) [50, 54) 
%v7 : [12, 46) 
%v8 : [14, 16) 
%v9 : [20, 22) 
%v10 : [36, 44) 
%v11 : [38, 44) 
%v12 : [40, 44) 
%v13 : [26, 34) 
%v14 : [28, 34) 
%v15 : [30, 34) 
%v16 : [44, 50) 
%v17 : [44, 50) 
%v18 : [44, 50) 
%v19 : [46, 50) 
%v20 : [52, 54) 
%v21 : [54, 56) 
```

### Table 3 — NestedConditionInsideLoopGraph

| Instr | %v0 | %v1 | %v2 | %v3 | %v4 | %v5 | %v6 | %v7 | %v8 | %v9 | %v10 | %v11 | %v12 | %v13 | %v14 | %v15 | %v16 | %v17 | %v18 | %v19 | %v20 | %v21 |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| 2 | R0 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 4 | R0 | R1 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 6 | R0 | R1 | R2 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 8 | R0 | R1 | R2 | S0 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 10 | R0 | R1 | R2 | S0 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 12 |  |  |  |  | S2 | S3 | S1 | S4 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 14 |  |  |  |  | S2 | S3 | S1 | S4 | R2 |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 16 |  |  |  |  | S2 | S3 | S1 | S4 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 18 |  |  |  |  | S2 | S3 | S1 | S4 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 20 |  |  |  |  | S2 | S3 | S1 | S4 |  | R2 |  |  |  |  |  |  |  |  |  |  |  |  |
| 22 |  |  |  |  | S2 | S3 | S1 | S4 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 24 |  |  |  |  | S2 | S3 | S1 | S4 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 26 |  |  |  |  |  | S3 | S1 | S4 |  |  |  |  |  | R2 |  |  |  |  |  |  |  |  |
| 28 |  |  |  |  |  |  | S1 | S4 |  |  |  |  |  | R2 | R0 |  |  |  |  |  |  |  |
| 30 |  |  |  |  |  |  |  | S4 |  |  |  |  |  | R2 | R0 | R1 |  |  |  |  |  |  |
| 32 |  |  |  |  |  |  |  | S4 |  |  |  |  |  | R2 | R0 | R1 |  |  |  |  |  |  |
| 34 |  |  |  |  | S2 | S3 | S1 | S4 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 36 |  |  |  |  |  | S3 | S1 | S4 |  |  | R0 |  |  |  |  |  |  |  |  |  |  |  |
| 38 |  |  |  |  |  |  | S1 | S4 |  |  | R0 | R1 |  |  |  |  |  |  |  |  |  |  |
| 40 |  |  |  |  |  |  |  | S4 |  |  | R0 | R1 | R2 |  |  |  |  |  |  |  |  |  |
| 42 |  |  |  |  |  |  |  | S4 |  |  | R0 | R1 | R2 |  |  |  |  |  |  |  |  |  |
| 44 |  |  |  |  |  |  |  | S4 |  |  |  |  |  |  |  |  | R0 | R2 | R1 |  |  |  |
| 46 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | R0 | R2 | R1 | S5 |  |  |
| 48 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | R0 | R2 | R1 | S5 |  |  |
| 50 |  |  |  |  | S2 | S3 | S1 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| 52 |  |  |  |  |  |  | S1 |  |  |  |  |  |  |  |  |  |  |  |  |  | R0 |  |
| 54 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | R0 |