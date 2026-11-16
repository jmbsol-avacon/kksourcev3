# Kwik Kuote – Business Requirements
## Extracted from `KKUOTE.C` and `KK_MORE.C`

> **Source:** Reverse-engineered from the original DOS C codebase (~1993–1997).  
> **Purpose:** Human-readable specification to guide modernization.  
> **Scope:** Covers all business rules found in `KKUOTE.C` (5,329 lines) and `KK_MORE.C` (1,015 lines).

---

## Table of Contents

1. [System Variants](#1-system-variants)
2. [Core Data Entities](#2-core-data-entities)
3. [Job Lifecycle & Status Rules](#3-job-lifecycle--status-rules)
4. [Estimate (Quotation) Rules](#4-estimate-quotation-rules)
5. [Financial Calculations](#5-financial-calculations)
6. [Part List Rules](#6-part-list-rules)
7. [Purchase Order Rules](#7-purchase-order-rules)
8. [Tax Rules](#8-tax-rules)
9. [Vendor & Product Rules](#9-vendor--product-rules)
10. [Sales Person Rules](#10-sales-person-rules)
11. [Comments System](#11-comments-system)
12. [Document & Letter Generation](#12-document--letter-generation)
13. [Printing & Output Rules](#13-printing--output-rules)
14. [Information Menu Structure](#14-information-menu-structure)

---

## 1. System Variants

The application supports two distinct operating modes compiled in at build time:

| Variant | Description | Key Behavioral Differences |
|---|---|---|
| **Distributor** | `PROJKK.EXE` (default) | Bid includes markup; salesperson's total_sales updated on Won/Lost |
| **Representative** | Compiled with `REP_VER` flag | Bid = COGS + shipping only (no markup); commission based on vendor rate × COGS × SP rate |

Any rule below that differs between variants is explicitly noted.

---

## 2. Core Data Entities

| Entity | Code | Description |
|---|---|---|
| Client | CD | Companies that request quotes / are billed |
| Vendor | VD | Suppliers of products |
| Product | PD | Catalog items sold by a vendor |
| Job | JB | A construction/installation project |
| Estimate | ES | A bid/quotation for a job, associated with one client |
| Part List | PL | Line items (products + quantities) for an estimate |
| Purchase Order | PO | Formal order placed with a vendor for a won job |
| Sales Person | SP | Internal employee responsible for a job/estimate |
| Product Line | LI | Category grouping within a vendor's catalog |
| Product Class | CL | Sub-category grouping within a product line |
| Discount Schedule | DS | Price-break table for a vendor (by order value range) |
| Room Schedule | RS | Room-by-room quantity breakdown for an estimate |
| Installation | IN | Installer assignment records by day |
| User | US | System user accounts with privilege settings |

---

## 3. Job Lifecycle & Status Rules

### 3.1 Status Values

| Code | Meaning | Transitions Allowed From |
|---|---|---|
| `B` | Bid (active quote stage) | `A` only (reverting from Awarded) |
| `A` | Awarded (client selected this bid) | `B` |
| `W` | Won (contract confirmed) | `A` or `B` (auto when only 1 estimate) |
| `L` | Lost | `A` or `B` (auto when only 1 estimate) |
| `O` | On Order (PO created) | `W`, `S`, `P`, `R` |
| `P` | Partially Shipped | `O` |
| `S` | Fully Shipped | `O`, `P` |
| `R` | Released (for fabrication) | `O` |

### 3.2 Status Transition Rules

- **Default status** for a new job is `B` (Bid). If the status field is empty, it is set to `B` automatically on validation.
- A job **cannot be changed to Won or Lost** if it has multiple estimates and is still in `B` status — the user must first Award the job from the Quotation (Estimate) screen.
- If a job has **only one estimate**, it may be set directly to Won or Lost without going through the Award step.
- **Once Shipped (`S`)**, the status cannot be changed; the system blocks further status edits.
- When status is set to **Shipped (`S`)**, the `sent` (ship date) field is automatically set to today's date.
- **Reverting to Bid (`B`)** is only allowed if the current status is `A` (Awarded).
- A job **must have at least one Estimate** to change status to Won, Lost, or Awarded.

### 3.3 Status Synchronization

Whenever a Job's status changes, the **linked Estimate record's status is updated to match** the new Job status. This keeps Job and Estimate status in sync at all times.

### 3.4 Job Deletion Cascade

Deleting a Job record **cascades deletion** to all linked data in this order:
1. All **Purchase Orders** (PO) whose `jbcode` matches the deleted job
2. **Job Comments** (stored in `.JC` extension file)
3. All **Estimates** (ES) for this job
4. All **Part Lists** (PL) associated with each deleted estimate
5. All **Room Schedule** files (`<jbcode>.*`) for this job

---

## 4. Estimate (Quotation) Rules

### 4.1 Creating an Estimate

- An estimate is automatically created when a user navigates to the Estimate screen from a Job that has no estimates yet. The estimate is pre-populated with:
  - Job code and job name from the parent Job
  - State from the parent Job
  - Status from the parent Job
  - Estimate date set to today
  - Tax rate looked up from the tax table based on job state/city
- A user may also **manually create a new estimate** for an existing Job via the `F2` key:
  - The Job must be in `B` (Bid) status. Jobs already Awarded cannot receive new estimates.
  - The new estimate is inserted in sorted order within the estimate file.
  - The Part List index (`plvec`) is initialized to `0` (no part list assigned yet).
  - Client code is cleared, requiring the user to assign a client to the new estimate.

### 4.2 Awarding an Estimate (F7)

Awarding an estimate is the act of selecting one winning bid. The rules are:

1. A valid, defined **Client** must be set on the estimate before awarding.
2. The parent **Job must be in `B` (Bid) status**. If the job is already awarded, the action is blocked with an error.
3. On award:
   - The awarded estimate's `award` flag is set to `YES` and status set to `A`.
   - **All other estimates for the same job are deleted** (their records are physically removed from the file).
   - **Part lists belonging to non-awarded estimates are deleted**.
   - The Job record's status is set to `A` (Awarded).
   - The Job record's `esvec` field is updated to point to the winning estimate's index.
   - The estimate date is updated to today.

### 4.3 Deleting an Estimate (F3)

- If the deleted estimate has a Part List (`plvec > 0`) and a Client assigned, the associated Part List file is also deleted.
- The change flag is reset to `NO` after deletion (prevents "save changes?" prompt).

### 4.4 Estimate Navigation & Linking

- The system remembers the **last job accessed** (`lastjob` global). Navigating to the PO or Estimate screen automatically positions the cursor at the last-used job's record.
- From the Job screen, pressing the estimates hotkey (`F4` / jb_es) opens the Estimate screen positioned on the first estimate for that job.
- After returning from the Estimate screen, the Job screen cursor is updated to reflect any job that was last modified.

### 4.5 Client Assignment Rules

- Once a client code is set on an estimate, it **cannot be changed** using the normal field entry. To assign a different client, a new estimate must be created via `F2`.
- A client code entered on the estimate must exist in the Client (CD) database. Invalid codes are rejected.
- When a valid client is selected, the estimate's phone number (`esttel`) is auto-populated from the client record.

---

## 5. Financial Calculations

### 5.1 Core Estimate Financial Fields

| Field | Description |
|---|---|
| `cogs` | Cost of Goods Sold — total raw product cost |
| `mkup` | Total markup dollars |
| `mkuprate` | Markup rate as a ratio (mkup / cogs) |
| `esttax` | Estimated tax amount |
| `estship` | Estimated shipping / freight cost |
| `taxable` | Subtotal of amounts subject to tax |
| `taxrate` | Tax rate (decimal, e.g., 0.08 = 8%) |
| `bid` | Final bid price presented to client |
| `subtotal` | COGS + markup (before tax and shipping) |
| `com` | Commission amount |
| `profit` | Markup minus commission |

### 5.2 Re-Calculation Algorithm (`re_calc`)

The estimate totals are recalculated from the Part List. The algorithm processes vendors group by group:

1. **For each vendor group** in the Part List:
   - Sum `COGS += qty × price` for all items in the group.
   - Sum `Weight += qty × weight`.
   - Calculate estimated shipping:
     - If freight is taxable (system setting): `Shipping += qty × freight_rate × weight × (1 + tax_rate)`
     - Otherwise: `Shipping += qty × freight_rate × weight`
   - If the vendor is marked **Taxable** (`vd.taxable = 'Y'`): `Taxable Amount += (qty × price) + (qty × price × markup%/100)`
   - Otherwise: taxable amount contribution from this vendor = 0.

2. **Markup** per vendor group = `qty_price × markup_rate / 100`

3. **Aggregate totals** across all vendor groups:
   - `Total Markup = sum of per-vendor markups`
   - `Markup Rate = Total Markup / Total COGS`
   - `Estimated Tax = Taxable Amount × Tax Rate`

4. **Commission** calculation differs by variant:
   - Distributor: `Commission = Total Markup × Salesperson Rate`
   - Representative: `Commission = Vendor Commission Rate × COGS × Salesperson Rate`

5. **Bid Price** calculation differs by variant:
   - Distributor: `Bid = COGS + Estimated Tax + Markup + Estimated Shipping`
   - Representative: `Bid = COGS + Estimated Shipping` (no markup added to bid)

6. **Subtotal** = `COGS + Markup` (same for both variants)
7. **Profit** = `Markup - Commission`

### 5.3 Bid Price Rounding

When rounding is requested (`rd = TRUE`):

1. Compute the preliminary bid price as above.
2. Round the bid to the nearest whole dollar.
3. **Back-calculate the markup rate** needed to achieve that rounded bid:
   - If vendor is **not taxable**: `markUp = (roundedBid - COGS - existingTax - existingMarkup - shipping) / qty_price`
   - If vendor **is taxable**: `markUp = (roundedBid - COGS - existingTax - existingMarkup - shipping - qty_price × taxRate) / (qty_price × (1 + taxRate))`
4. **Negative markup is never allowed.** If back-calculation yields a negative markup, the rounded bid target is incremented by $1 and recalculated.
5. The revised markup rate is written back to all Part List records in the vendor group.

### 5.4 Manual Recalculation (F4 in Estimate Screen)

The user may trigger a recalculation via `F4`. Two paths are available:

**Path A — Recalculate from Part List:**
- User selects which vendor(s) to include (All or specific vendor).
- System runs full `re_calc` with rounding enabled.
- All financial fields on the estimate are updated.

**Path B — Recalculate from Current ES Fields (without Part List):**
- Commission = `mkup × SP rate`
- Tax = `(cogs + mkup) × taxrate`  *(Note: uses subtotal for tax base in this path)*
- Distributor Bid = `cogs + esttax + mkup + estship`
- Rep Bid = `cogs + estship`
- Subtotal = `cogs + mkup`
- Profit = `bid - (commission + tax + shipping + cogs)`

### 5.5 Quick Price Check Calculator (`pl_total`)

A standalone calculator available from the Part List screen. The user enters:
- **Markup %** and **Tax %**

The system displays (updating in real time):
- COGS (from PL totals)
- Total Markup = `COGS × (markup% / 100)`
- Estimated Tax = `(COGS + Total Markup) × (tax% / 100)`
- Estimated Shipping (from PL freight totals)
- **Bid Price** = `COGS + Total Markup + Estimated Tax + Estimated Shipping`

---

## 6. Part List Rules

### 6.1 Part List Identity & File Naming

Each Part List is uniquely identified by three pieces of data from its Estimate:
- **Job Code** (`jbcode`) — links to the Job
- **Client Code** (first 2 characters) — links to the Client
- **PL Vector** (`plvec`) — a digit 1–9, allowing up to 9 part lists per job/client combination

The Part List filename is: `<jbcode>.<client[0]><client[1]><plvec>` (e.g., `00000042.AB1`)

A `plvec` of `0` means the estimate has no part list yet.

### 6.2 Part List Creation

- A Part List is created the first time the user opens the Part List screen (`F8`) from an Estimate that has no PL yet.
- The system finds the **next available PL index** (1–9) not already used by other estimates for the same job and client.
- If other estimates for the same job exist, the Part List is **automatically copied** from the most recently found sibling estimate's PL as a starting point.

### 6.3 Part List Item Fields

| Field | Description |
|---|---|
| `flg` | Record validity flag (0=empty, 2=valid) |
| `qty` | Quantity ordered |
| `vdcode` | Vendor code (must exist in VD database) |
| `pdcode` | Product code |
| `description` | Product description (truncated to 23 characters) |
| `price` | Unit price |
| `f` | Freight rate (shipping rate per lb) |
| `weight` | Unit weight |
| `markup` | Markup percentage for this item's vendor group |

### 6.4 Part List Validation

A PL record is considered **valid** (`flg = 2`) only if both `pdcode` and `vdcode` are non-empty.
If either is missing, `flg` is set to `0` (invalid/empty).

### 6.5 Vendor Entry in Part List

- When entering a vendor code on a PL line, the code is converted to uppercase.
- The vendor must exist in the VD database. Invalid vendor codes are rejected and the prior value is restored.
- If the vendor code changes, the product code and price fields are automatically cleared.
- The **last-used vendor code** is remembered globally and used as the default for the next new PL line.

### 6.6 Product Lookup from Part List

- Pressing `F1` on the product code field opens a product lookup list for the current vendor.
- Selecting a product auto-fills: product code, description (max 23 chars), list price, freight rate, weight.
- In the **Rep version**, the vendor's commission rate is also loaded into the markup field.

### 6.7 Product Price Tiers (F9)

The user can bulk-update prices on PL records from the product catalog using one of four price tiers:

| Option | Source Field |
|---|---|
| List Price | `pd.p1` |
| Dealer Price | `pd.p2` |
| Net Price | `pd.p3` |
| Sales Price | `pd.sales` |

This can be applied to a single record or all records for a selected vendor group.

### 6.8 Vendor Discount (F4)

A percentage discount can be applied to all prices for a vendor group:

- **Manual discount:** User enters a percentage directly.
- **Schedule-based discount:** System looks up the vendor's Discount Schedule (DS file):
  1. Calculates the total price of the vendor's items in the PL.
  2. Finds the matching dollar range bracket in the DS file.
  3. The discount percentage from that bracket is applied.
  - Schedule-based discounts **cannot be applied to multiple vendors at once**.
- Applied as: `new_price = old_price × (1 - discount / 100)`

### 6.9 Vendor Cross-Reference (F7)

Replaces products from one vendor with equivalent products from another vendor:

1. User selects a target vendor to cross-reference to.
2. The system filters the vendor list to show only vendors with the same **product category** (`vd.product`) as the source vendor.
3. For each PL item matching the source vendor, the system searches the target vendor's product catalog for a matching `ref` (cross-reference) code.
4. If a match is found (case-insensitive), the user can choose `R` (Replace) or `N` (Next).
5. On replacement: vendor code, product code, description, price, and freight rate are updated from the new vendor's product record.
6. Products with an empty `ref` field **cannot be cross-referenced** (no cross-reference available).

### 6.10 Part List Totals (F8)

- Shows totals for a selected vendor or all vendors.
- Displays: **Total Price** and either **Total Weight** or **Total Shipping** (user can toggle between weight and shipping display via Alt-S).
- Format: `Total price (<vdcode>): $X.XX  Total Weight: X.XX lbs` or `Total Shipping: X.XX`

### 6.11 Product Super-Help (Shift-F1 / Alt-F1)

Allows finding products by Line × Class hierarchy:
1. User selects a **Product Line** from the vendor's LI file.
2. If the line starts with `'0'`, the Class selection is skipped (wildcard class `*` is used).
3. User selects a **Product Class** from the vendor's CL file.
4. System builds a temporary file containing all products matching both the selected Line and Class.
5. User picks from this filtered product list.
6. `Shift-F1` re-uses the last selected Class, skipping to the class picker.

### 6.12 Separator Records

Part List vendor groups can be separated by **special separator records** with vendor codes starting with `__` (double underscore). These are always skipped in totaling and group-boundary calculations.

---

## 7. Purchase Order Rules

### 7.1 PO Creation Prerequisites

A Purchase Order **cannot be created** unless:
1. The Job has at least one Estimate.
2. The Job status is one of: `W` (Won), `S` (Shipped), `P` (Partially Shipped), `O` (On Order), or `R` (Released).
3. The Estimate has a Part List with at least one vendor.

If the job has only a single estimate and status is not Shipped/On-Order/Released, the status is automatically set to `W` (Won) when PO creation is triggered.

### 7.2 PO Auto-Numbering

- PO numbers are **8-digit zero-padded sequential numbers** (e.g., `00000042`).
- The next PO number is determined by reading the last record in the PO file and incrementing by 1.
- Each vendor group in the Part List generates a **separate PO** with its own incremented number.

### 7.3 PO Financial Data

When a PO is created, its financial fields are populated from the Estimate:

| PO Field | Source |
|---|---|
| `cogs` | `es.cogs` |
| `commission` | Recalculated as `profit × salesperson_rate` |
| `tax` | `es.esttax` |
| `estship` | `es.estship` |
| `bid` | `es.bid` |
| `mkup` | `es.mkup` |
| `profit` | `mkup - (if shipped: actcogs - cogs) + (if shipping ≠ 0: estship - shipping)` |
| `plidx` | Index of the vendor group in the PL (1-based) |

### 7.4 Side Effects of PO Creation

When a PO is created for a vendor group:
1. **Job status** → `O` (On Order)
2. **Estimate status** → `O` (On Order)  *(both updated simultaneously)*
3. **Client YTD On Order** (`cd.ytd_on_order`) += bid amount
4. **Vendor YTD On Order** (`vd.ytd_on_order`) += COGS amount
5. If the Job's **Project Manager** field (`agent`) is empty, it is set from the estimate's `estname`.
6. The PO date (`podate`) is set to today.
7. The ship date (`shipdate`) is copied from the Job's `sent` field.

### 7.5 Vendor Group Locking

- Each vendor group in the PL is tracked by the PO's `plidx` field.
- Once a PO exists for a vendor group (`plidx` match), that group is **locked** — it cannot be selected for new PO creation.
- A vendor group with an existing PO shows as "not allowed to tag" in the vendor selection window.

### 7.6 Change Order POs

- When creating a **Change Order PO** (vs a regular PO), the PO's `jbcode2` field stores the original job code for reference.
- Regular POs: `jbcode2` = current job code
- Change Order POs: `jbcode2` = the change order code (`jb.ch_od_code`)

### 7.7 PO Status Display

The message bar shows a human-readable status for each PO based on its status code and dates:

| Status | Ship date in future | Ship date in past or empty |
|---|---|---|
| `S` (Shipped) | "Will Ship on [date]" | "Shipped on [date]" |
| `O` (On Order) | "Placed On Order on [podate], and will Ship on [shipdate]" | "Placed On Order on [podate]" |
| `P` (Partial) | "Will Ship Partially on [date]" | "Partially Shipped on [date]" |
| `R` (Released) | "Will Release on [release_date]" | "Released on [release_date]" |

---

## 8. Tax Rules

### 8.1 Tax Table Lookup

- The system maintains a **Tax Table** (`TXREC` records, `TX` file) indexed by state/city combinations.
- When an Estimate is created or a Job's state changes, `set_taxrate()` is called:
  1. Load the tax table from disk into memory (`tx[]` array).
  2. Search for a matching record where `tx[i].state == jb.state` **and** `tx[i].city == jb.city`.
  3. If found: `es.taxrate = tx[i].txrate`.
  4. If no city match: fall back to the first record where only `tx[i].state` matches.
  5. If no state match at all: `es.taxrate = 0.0` (no tax).
- The tax table is loaded once and kept in a static memory buffer; freed on application exit (`free_tx()`).

### 8.2 Taxable Freight

- A **system-level setting** (`sysvar.taxable_freight`) controls whether freight/shipping costs are included in the taxable base.
- If `taxable_freight = YES`: shipping cost is inflated by `(1 + taxrate)` before being added to estimated shipping.
- If `taxable_freight = NO`: shipping is added at face value with no tax component.

### 8.3 Vendor-Level Taxability

- Each Vendor record has a `taxable` flag (`Y` / `N`).
- Only products from **taxable vendors** contribute to the `taxable` subtotal used to compute `esttax`.
- Products from non-taxable vendors never attract sales tax regardless of the estimate's tax rate.

### 8.4 Tax Display on Estimate

- The tax rate is displayed as a percentage (e.g., `8.25%`).
- The estimated tax dollar amount (`esttax`) is displayed next to the rate.
- Both fields are **read-only** if auto-calculated from the Part List; they become editable in the "recalculate from fields" path.

---

## 9. Vendor & Product Rules

### 9.1 Vendor Record Key Fields

| Field | Description |
|---|---|
| `vdcode` | 8-char unique vendor identifier |
| `vdname` | Vendor company name |
| `product` | Product category code (used for cross-reference grouping) |
| `taxable` | `Y` / `N` — whether vendor's products are taxable |
| `markup` | Default markup percentage |
| `commission` | Default commission rate (Rep version only) |
| `ytd_on_order` | Year-to-date on-order dollars (COGS) |
| `ytd_sales` | Year-to-date shipped/invoiced dollars |
| `freight` | Freight rate ($ per lb) |

### 9.2 Vendor Markup Rate (`set_rate`)

- The **Markup/Commission Rate** screen shows all vendors present in the current estimate's Part List.
- Each vendor shows its current markup rate read from the first PL record in that vendor's group.
- User may change any vendor's rate; on save, **all PL records for that vendor** in this estimate are updated with the new rate.

### 9.3 Vendor Selection Window (multi-tag)

When generating POs or cross-referencing, the user sees a **Vendor Selection Window**:
- **Tag individual vendors** (`F7` or `Space`) — toggles tag; cursor advances to next.
- **Tag All** (`F8`) — tags every vendor.
- **Untag All** (`F6`) — removes all tags.
- **Select one** (`Enter`) — tags only the vendor under the cursor.
- **Cancel** (`Esc`) — aborts with no selection.
- Vendors that already have a PO are marked "not allowed to tag."

### 9.4 Product Cross-Reference

- Cross-referencing is vendor-category scoped: only vendors sharing the same `product` category code appear as candidates.
- Matching is done on the `ref` field of both the source PL item and target PD catalog entry (case-insensitive).
- A product with an empty `ref` field cannot be cross-referenced.

### 9.5 Discount Schedules

- A vendor may have a **Discount Schedule** (`DS` file): a table of `(low_value, high_value, discount%)` tiers.
- The applicable tier is found by summing all of that vendor's PL line-item totals and matching the dollar range.
- Discount schedules **only apply to a single vendor group** at a time.

### 9.6 Product Lines and Classes (LI / CL Files)

- Each vendor can have a **Product Line** file (`LI`) with `licode` and `liname`.
- Each vendor can have a **Product Class** file (`CL`) with `clcode`, `clname`, and `licode`.
- Products (`PD`) belong to both a Line and a Class via their `li` and `cl` fields.
- The Super-Help product picker (`Shift-F1`) uses these hierarchies to filter the product list.

---

## 10. Sales Person Rules

### 10.1 Sales Person Fields

| Field | Description |
|---|---|
| `spcode` | Unique salesperson code |
| `spname` | Full name |
| `rate` | Commission rate (fraction, e.g., 0.20 = 20%) |
| `total_sales` | Year-to-date total sales dollars (Distributor only) |

### 10.2 Commission Calculation

**Distributor version:**
- `Commission = Markup x salesperson.rate`
- `salesperson.total_sales` is incremented when a Job is set to **Won** or **Lost**.

**Representative version:**
- `Commission = vendor.commission_rate x COGS x salesperson.rate`
- No `total_sales` tracking in Rep version.

### 10.3 Salesperson Assignment

- A Job record stores the `spcode` of the responsible salesperson.
- If a Job's `agent` (Project Manager) field is blank when the first PO is created, it is auto-populated from the Estimate's `estname` contact field.

---

## 11. Comments System

### 11.1 Comment Storage

- **Job Comments** are stored in a file with extension `.JC` named after the job code (e.g., `00000042.JC`).
- **Estimate Comments** are stored inline as `ESREC.comments` — a text array embedded in the Estimate record.
- **PO Comments** use a `.PC` extension file.

### 11.2 Comment Lifecycle

- Job Comments are **deleted** automatically when the parent Job is deleted (cascade).
- Estimate Comments are included in the printed quote/proposal document.

---

## 12. Document & Letter Generation

### 12.1 PO Print (`po_print`)

A Purchase Order document printed to the default printer includes:
- Company header (from `compInfo` system settings)
- PO number, date, vendor address
- Line-item table: Qty | Product Code | Description | Unit Price | Extended Price
- Totals: COGS, Estimated Shipping, Tax, Bid Price
- Signature / authorization block

### 12.2 PO Fax (`po_fax`)

Same content as PO Print but routed through the fax subsystem (`DB_FAX.C`).
The vendor's fax number from the VD record is used as the destination.

### 12.3 Mailing Labels (`print_label_to_label_sheet`)

- Prints to a **30-up label sheet** (3 columns x 10 rows).
- User enters a grid coordinate (e.g., `2,5` = column 2, row 5).
- Row offset = `(row - 1) x 6` blank lines prepended.
- Column offset uses tabstop jumps for columns 2 and 3.
- Content: Name, Company, Address, City/State/ZIP, optional RE/Attention line.

### 12.4 Envelope Printing (`print_address_on_envelop`)

- Prints in **landscape** orientation.
- Adds 6 blank lines for vertical positioning.
- Job envelopes: company name prefixed with `c/o:`, attention line labeled "Attention:".
- Non-Job envelopes: company name printed directly, attention line labeled "RE:".

---

## 13. Printing & Output Rules

### 13.1 Printer Codes

All printing uses Epson ESC/P codes stored in a `PRNCODES` struct (`prnCodes`):

| Code | Purpose |
|---|---|
| `reset` | Reset printer to defaults |
| `land` | Switch to landscape orientation |
| `bold` / `bold_off` | Bold text on/off |
| `compress` | Compressed (condensed) pitch |
| `expand` | Double-width / expanded text |
| `underline` / `underline_off` | Underline on/off |

### 13.2 Report Line Width

- Standard reports: **80-column** layout (portrait).
- PO and proposal documents: **132-column** layout (landscape, compressed pitch).
- Label sheets: managed with tab stops and `%-25.25s` fixed-width formatting.

---

## 14. Information Menu Structure

The application's main information access follows a hierarchical menu:

```
Main Menu
+-- Estimates (Quotation Management)
|   +-- View/Edit Estimates
|   +-- Award Estimate (F7)
|   +-- Create new Estimate for Job (F2)
|   +-- Delete Estimate (F3)
|   +-- Part List (F8)
|   |   +-- Add/Edit line items
|   |   +-- Vendor Discount (F4)
|   |   +-- Price Tier Update (F9)
|   |   +-- Cross-Reference Vendor (F7)
|   |   +-- PL Totals (F8)
|   |   +-- Product Super-Help (Shift-F1)
|   +-- Recalculate (F4)
+-- Jobs
|   +-- View/Edit Jobs
|   +-- Change Status
|   +-- Link to Estimates (F4)
|   +-- Delete Job (cascades)
+-- Purchase Orders
|   +-- Create PO from Part List
|   +-- Print PO / Fax PO
|   +-- View PO Status
+-- Clients (CD)
+-- Vendors (VD)
|   +-- Products (PD), Product Lines (LI), Product Classes (CL)
|   +-- Discount Schedules (DS)
+-- Sales Persons (SP)
+-- Rooms / Installation (RS / IN)
+-- Reports
+-- System Setup
    +-- Users (US), Tax Table (TX), System Variables (SY), Company Info
```

---

*Document generated by reverse-engineering `KKUOTE.C` (5,329 lines) and `KK_MORE.C` (1,015 lines). Last updated: 2026-05-07.*
