# A. "The three big players"

## I. Entities

Entities bestehen aus nichts weiter als Metadaten. Dazu geh�ren:
- ein eindeutiger `dataIndex`, welcher den Komponenten zugeordnet wird,
- eine Variable `alive`, welche angibt, ob die Entity aktiv ist und
- ein Bitset, um der Entity Komponenten zuzuordnen.

## II. Komponenten

Komponenten sind nichts anderes als Objekte, die keine Logik besitzen.
Idealerweise handelt es sich um einfache PODs.

Jeder Entity k�nnen mehrere Typen von Komponenten zugerordnet werden, um eine Art
von Eigenschaft bereitzustellen. So kann eine "Gesundheitskomponente" einer Entity
zugeordnet werden, um sie sterblich zu machen, indem sie ihr "Gesundheit" gibt, die
nicht mehr als ein Integer- oder Gleitkommawert im Speicher ist.

```cpp
struct HealthComponent
{
    int health{ 0 };
};

struct CircleComponent
{
    float radius{ 0 };
};

struct InputComponent
{
    int key{ 0 };
};
```

## III. Systeme

todo

____

# B. Beispiel

## I. Compile-time settings

Alle Komponenten und Signaturen sind bereits zur Kompilierungszeit bekannt. Es m�ssen
entsprechende Listen erstellt und an eine `Settings` Klasse �bergeben werden.

```cpp
//-------------------------------------------------
// Define components && component list
//-------------------------------------------------

struct HealthComponent
{
    int health{ 0 };
};

struct CircleComponent
{
    float radius{ 0 };
};

struct InputComponent
{
    int key{ 0 };
};

using MyComponentsList = ComponentList<HealthComponent, CircleComponent, InputComponent>;

//-------------------------------------------------
// Define signatures && signature list
//-------------------------------------------------

using SignatureVelocity = Signature<InputComponent, CircleComponent>;
using SignatureLife = Signature<HealthComponent>;

using MySignaturesList = SignatureList<SignatureVelocity, SignatureLife>;

//-------------------------------------------------
// Create `Settings` with above two compile-time lists
//-------------------------------------------------

using MySettings = Settings<MyComponentsList, MySignaturesList>;
```

Mit den so erstellten Alias `MySettings` kann der `Manager` erstellt werden.

```cpp
//-------------------------------------------------
// Create `Manager` with above compile-time `Settings`
//-------------------------------------------------

using MyManager = Manager<MySettings>;
```

## II. Runtime settings

Es wird eine Instanz von `MyManager` erzeugt.

```cpp
MyManager manager;
```

Mit dem `manager` k�nnen jetzt zur Laufzeit u.a. Entities erstellt und mit Komponenten
verbunden werden.

```cpp
// create entity
const auto i0 = manager.CreateIndex();

// add a component
auto& healthComponent{ manager.AddComponent<HealthComponent>(i0) };
healthComponent.health = 80;
```
___

# C. Implementation

## I. Entity

### 1. CreateIndex

Eine neue Entity kann mit `const auto i0 = manager.CreateIndex();` erstellt werden.

Um alle Entities zusammenh�ngend zu speichern, enth�lt der `Manager` einen Member `m_entities`:

```cpp
/**
 * @brief The entities are stored contiguously in a `std::vector`.
 */
std::vector<Entity> m_entities;
```

Der Konstruktor von `Manager` f�hrt einen `resize()` auf `m_entities` mit dem Wert 100 (DEFAULT_ENTITY_CAPACITY) aus.
Nach dem Start befinden sich demnach 100 "tote" Entities in `m_entities`. Die Methode `CreateIndex()` holt sich
nun den n�chsten freien Index (die n�chste "tote" Entity) und �ndert deren Status auf `alive = true`.
`CreateIndex()` erstellt also nichts, sondern �ndert nur bereits vorhandene Daten.

```cpp
const auto i0 = manager.CreateIndex();
```

```
m_entities = 
----------------------------------------
|  i0  | Index1 | Index2 | usw. | usw. |
----------------------------------------
    |        |        |
    |        |    [ Entity]
    |        |      - dataIndex = 2
    |        |      - alive = false
    |        |      - bitset = alles auf 0
    |        |
    |    [ Entity ]
    |      - dataIndex = 1
    |      - alive = false
    |      - bitset = alles auf 0
    |
[ Entity ]
  - dataIndex --> wird beim Erstellen der Entity auf den Wert von Index gesetzt (hier auf 0)
  - alive = ist jetzt true
  - bitset = alles auf 0
```

### 2. Refresh

Die Methode ordnet die Entities in `m_entities` so neu an, dass sich links alle "lebenden" und rechts alle "toten" befinden.
Die zusammenh�ngende Speicherung von "lebenden" Entities vereinfacht u.a. den Zugriff. Jede Entity hat einen festen
`dataIndex`, welcher f�r die Zuordnung zu den Komponenten benutzt wird. Die Neuanordnung in `m_entities` hat darauf keine
Auswirkung.

Vor `Refresh()`:

```
a = alive
d = dead
n = new && alive

2 Entities wurden deaktiviert (E1 und E3)
3 Entities wurde neu erstellt (E5 - E7)

Das ergibt folgenden Bild vor dem Refresh():

-------------------------------------------------
| E0 | E1 | E2 | E3 | E4 | E5 | E6 | E7 |   |   |
-------------------------------------------------
  a    d    a    d    a  | n    n    n  | d   d |
                         |              |       |
                 m_size  /              |       |
                                        |       |
                            m_sizeNext  /       |
                                                |
                                    m_capacity  /


Nach `Refresh()`:

-------------------------------------------------
| E0 | E7 | E2 | E6 | E4 | E5 | E3 | E1 |   |   |
-------------------------------------------------
  a    a    a    a    a    a  | d    d    d   d |
                              |                 |
                      m_size  /                 |
                  m_sizeNext                    |
                                                |
                                    m_capacity  /
```

Damit kann einfach �ber alle "lebenden" Entities iteriert werden.

Beispiel:

```cpp
for (EntityIndex index{ 0 }; index < m_size; ++index)
{
    // ...
}
```

## II. Komponenten

### 1. AddComponent

Die Zuordnung einer Komponente zu einer Entity erfolgt mit:

```cpp
// create entity
const auto i0 = manager.CreateIndex();

// add a component
auto& healthComponent{ manager.AddComponent<HealthComponent>(i0) };
healthComponent.health = 80;
```

***Folgendes passiert jetzt in AddComponent:***

a) `AddComponent()` pr�ft zun�chst, ob sich die Komponente in der Komponentenliste der `Settings`
Klasse befindet - also g�ltig ist.
b) Anschlie�end wird das KomponentenBit der Komponente geholt. Das Bit entspricht der KomponentenId, welche
wiederum der Position der Komponente in der Komponentenliste entspricht. Das Bitset der Entity wird dieser
Position gesetzt.
c) Schlie�lich wird die Komponente aus dem `m_componentStorage` geholt und mittels "placement new" neu erstellt.

***ComponentStorage***

`ComponentStorage` ist ein `std::tuple` mit einem `std::vector` f�r jeden einzelnen Komponententyp.

Beispiel:

```cpp
std::tuple<std::vector<HealthComponent>, std::vector<CircleComponent>, std::vector<InputComponent>
```

Auf die Elemente der `std::vector` Typen wird mit dem Entity `dataIndex` zugegriffen. Auf die `std::vector`
Typen wird ein `resize()` ausgef�hrt, wenn dies f�r `m_entities` notwendig ist.

____

# D. API

## I. Manager

***public***

`auto IsAlive(const EntityIndex entityIndex)`: Pr�ft, ob die Entity "lebt".

`void Kill(const EntityIndex entityIndex)`: Nach dem Aufruf ist die Entity "tot".

`auto CreateIndex()`: Erstellt eine neue Entity.

`void Clear()`: Nach dem Aufruf sind alle Entities "tot", alle Bitsets gel�scht und alle Variablen zur�ckgesetzt.

`void Refresh()`: Ordnet die Entities neu an: Links alle "lebenden" und rechts alle "toten".

`auto& AddComponent<TComponent>(const EntityIndex entityIndex, TArgs&&... args)`: Verbindet die Komponente mit einer Entity.

`bool HasComponent<TComponent>(const EntityIndex entityIndex)`: Pr�ft, ob die Entity einer Komponente zugeordnet ist.

`void DeleteComponent(const EntityIndex entityIndex)`: L�scht die Verbindung zwischen Komponente und Entity.

`void ForEntities(TCallable&& callable)`: 

`void ForEntitiesMatching<TSignature>(TCallable&& callable)`:

`std::size_t GetEntityCount()`: Gibt die Anzahl "lebender" Entities zur�ck.

`void PrintState(std::ostream& oss)`: Ausgabe von Debug-Infos.

***private***

`void GrowTo(std::size_t newCapacity)`: `resize()` auf `m_entities` und alle `std::vector` in `ComponentStorage`.

`void GrowIfNeeded()`: Pr�ft, ob `GrowTo()` ausgef�hrt werden muss.

`auto& GetEntity(const EntityIndex entityIndex)`: Gibt eine Referenz auf eine Entity zur�ck.

`const auto& GetEntity(const EntityIndex entityIndex)`: Gibt eine const Referenz auf eine Entity zur�ck.

`EntityIndex ArrangeAliveEntitiesToLeft()`: Ordnet die Entities neu an (links "lebende", rechts "tote").