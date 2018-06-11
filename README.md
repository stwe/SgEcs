# A. "The three big players"

## I. Entities

Entities bestehen aus nichts weiter als Metadaten. Dazu gehören:
- ein eindeutiger `dataIndex`, welcher den Komponenten zugeordnet wird,
- eine Variable `alive`, welche angibt, ob die Entity aktiv ist und
- ein Bitset, um der Entity Komponenten zuzuordnen.

## II. Komponenten

Komponenten sind nichts anderes als Objekte, die keine Logik besitzen.
Idealerweise handelt es sich um einfache PODs.

Jeder Entity können mehrere Typen von Komponenten zugerordnet werden, um eine Art
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

Systeme enthalten die Logik und gehen nur mit solchen Komponenten um, mit denen
sie sich befassen sollen (Signatur). Jede Signatur besteht aus einer Liste von
verschiedenen Komponenten. Eine Entity entspricht dann einer Signatur, wenn sie
alle Komponenten enthält, die von der Signatur benötigt werden.

____

# B. Beispiel

## I. Compile-time settings

Alle Komponenten- und Signaturlisten sind bereits zur Kompilierungszeit bekannt. Es müssen
die entsprechenden Listen erstellt und an eine `Settings` Klasse übergeben werden.

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

Mit dem so erstellten Alias `MySettings` kann der `Manager` erstellt werden.

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

Mit dem `manager` können jetzt zur Laufzeit u.a. Entities erstellt und diese mit Komponenten
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

Eine neue Entity kann z.B. mit `const auto i0 = manager.CreateIndex();` erstellt werden.

Um alle Entities zusammenhängend zu speichern, enthält der `Manager` einen Member `m_entities`:

```cpp
/**
 * @brief The entities are stored contiguously in a `std::vector`.
 */
std::vector<Entity> m_entities;
```

Der Konstruktor von `Manager` führt einen `resize()` auf `m_entities` mit dem Wert 100 (DEFAULT_ENTITY_CAPACITY) aus.
Nach dem Start befinden sich demnach 100 "tote" Entities in `m_entities`. Die Methode `CreateIndex()` holt sich
nun den nächsten freien Index (die nächste "tote" Entity) und ändert deren Status auf `alive = true`.
`CreateIndex()` erstellt also nichts, sondern ändert nur bereits vorhandene Daten.

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
Die zusammenhängende Speicherung von "lebenden" Entities vereinfacht u.a. den Zugriff. Jede Entity hat einen festen
`dataIndex`, welcher für die Zuordnung zu den Komponenten benutzt wird. Die Neuanordnung in `m_entities` hat darauf keine
Auswirkung.

Vor `Refresh()`:

```
a = alive
d = dead
n = new && alive

2 Entities wurden deaktiviert (E1 und E3)
3 Entities wurde neu erstellt (E5 - E7)

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

Damit kann einfach über alle "lebenden" Entities iteriert werden.

Beispiel:

```cpp
for (EntityIndex index{ 0 }; index < m_size; ++index)
{
    // ...
}
```

## II. Komponenten

Die Zuordnung einer Komponente zu einer Entity erfolgt mit:

```cpp
// create entity
const auto i0 = manager.CreateIndex();

// add a component
auto& healthComponent{ manager.AddComponent<HealthComponent>(i0) };
healthComponent.health = 80;
```

***Folgendes passiert jetzt in AddComponent:***

a) `AddComponent()` prüft zunächst, ob sich die Komponente in der Komponentenliste der `Settings`
Klasse befindet - also gültig ist.

b) Anschließend wird das KomponentenBit der Komponente geholt. Das Bit entspricht der KomponentenId, welche
wiederum der Position der Komponente in der Komponentenliste entspricht. Das Bitset der Entity wird an dieser
Position gesetzt.

c) Schließlich wird die Komponente aus dem `m_componentStorage` geholt und mittels "placement new" neu erstellt.

***ComponentStorage***

`ComponentStorage` ist ein `std::tuple` mit einem `std::vector` für jeden einzelnen Komponententyp.

Beispiel:

```cpp
std::tuple<std::vector<HealthComponent>, std::vector<CircleComponent>, std::vector<InputComponent>
```

Auf die Elemente der `std::vector` Typen wird mit dem Entity `dataIndex` zugegriffen.

```cpp

-----------------------------------------------------------------------------------
                       | <HealthComponent> | <CircleComponent> | <InputComponent> |
-----------------------------------------------------------------------------------
| Entity->dataIndex #0 |       x           |                   |                  |
| Entity->dataIndex #1 |                   |        x          |                  |
| Entity->dataIndex #2 |                   |        x          |        x         |
| Entity->dataIndex #3 |       x           |                   |                  |
| Entity->dataIndex #4 |                   |                   |        x         |
-----------------------------------------------------------------------------------
```

Auf die `std::vector` Typen wird dann ein `resize()` ausgeführt, wenn dies für `m_entities` notwendig ist.

## III. Systeme

Ein System wird nur dann aktiv, wenn der Entity alle notwendigen Komponenten zugeordnet sind.
Um das festzustellen, wird über alle "lebenden" Entities iteriert und geprüft, ob die angegebene
Signatur passt. In diesem Fall hat das System Zugriff auf die Komponenten der Entity - aber nicht
auf alle, sondern nur auf solche, die gleichzeitig die Signatur definieren.

```cpp
manager.ForEntitiesMatching<SignatureLife>
(
    [](auto entityIndex, HealthComponent& healthComponent)
    {
        healthComponent.health = 99;
    }
);

manager.ForEntitiesMatching<SignatureVelocity>
(
    [](auto entityIndex, InputComponent& inputComponent, CircleComponent& circleComponent)
    {
        inputComponent.key = 32;
        circleComponent.radius = 64.0f;
    }
);
```

Jede Signatur und jede Entity verfügt über ein `std::bitset`. Dabei ist jedes Bit für einen Komponententyp
reserviert. Welches Bit gesetzt wird, bestimmt die Id der Komponente. Um nun zu überprüfen, ob Entity und
Signatur übereinstimmen, wird ein bitweises UND verwendet.

In der Manager Klasse sieht das so aus:

```cpp

// ...

// Das Bitset der Signatur und das Bitset der Entity werden UND verknüpft:
return (signatureBitset & entityBitset) == signatureBitset;
```

Sollte das Ergebnis dieser Verknüpfung dem Bitset der Signatur entsprechen, erfüllt die Entity alle
Anforderungen der Signatur.

____

# D. API

## I. Manager

***public***

**`auto IsAlive(const EntityIndex entityIndex)`:** Prüft, ob die Entity "lebt".

**`void Kill(const EntityIndex entityIndex)`:** Nach dem Aufruf ist die Entity "tot".

**`auto CreateIndex()`:** Erstellt eine neue Entity.

**`void Clear()`:** Nach dem Aufruf sind alle Entities "tot", alle Bitsets gelöscht und alle Variablen zurückgesetzt.

**`void Refresh()`:** Ordnet die Entities neu an: Links alle "lebenden" und rechts alle "toten".

**`auto& AddComponent<TComponent>(const EntityIndex entityIndex, TArgs&&... args)`:** Verbindet die Komponente mit einer Entity.

**`bool HasComponent<TComponent>(const EntityIndex entityIndex)`:** Prüft, ob die Entity einer Komponente zugeordnet ist.

**`void DeleteComponent(const EntityIndex entityIndex)`:** Löscht die Verbindung zwischen Komponente und Entity.

**`auto& GetComponent<TComponent>(const EntityIndex entityIndex)`:** Gibt die Referenz auf eine Komponente zurück.

**`auto MatchesSignature<TSignature>(const EntityIndex entityIndex)`:** Prüft eine Entity gegen eine Signatur.

**`void ForEntities(TCallable&& callable)`:** Iteriert über alle "lebenden" Entities.

**`void ForEntitiesMatching<TSignature>(TCallable&& callable)`:** Iteriert über alle "lebenden" Entities, die mit einer bestimmten Signatur übereinstimmen.

**`std::size_t GetEntityCount()`:** Gibt die Anzahl "lebender" Entities zurück.

**`void PrintState(std::ostream& oss)`:** Ausgabe von Debug-Infos.

***private***

**`void GrowTo(std::size_t newCapacity)`:** `resize()` auf `m_entities` und alle `std::vector` in `ComponentStorage`.

**`void GrowIfNeeded()`:** Prüft, ob `GrowTo()` ausgeführt werden muss.

**`auto& GetEntity(const EntityIndex entityIndex)`:** Gibt eine Referenz auf eine Entity zurück.

**`const auto& GetEntity(const EntityIndex entityIndex)`:** Gibt eine const Referenz auf eine Entity zurück.

**`EntityIndex ArrangeAliveEntitiesToLeft()`:** Ordnet die Entities neu an (links "lebende", rechts "tote").

**`void ExpandSignatureCall(const EntityIndex entityIndex, TCallable&& callable)`:** Eine reine Hilfsfunktion.

