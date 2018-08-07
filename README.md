# QGeoJson Class Description:

The ***QGeoJson*** class provides a way to import and export a GeoJSON document to / from a custom QVariantMap.
A GeoJSON document can be converted from its text-based representation to a QVariantMap and viceversa using 2 public methods: 
1. QVariantMap ***importGeoJson***(const QJsonDocument &geojsonDoc);
2. QJsonDocument ***exportGeoJson***(const QVariantMap &geojsonMap);

The QvariantMap returned by the importer has a very specific architecture, shaped on the GeoJSON structure. 
The exporter accepts a QVariantMap with the same structure.

This QVariantMap always has at least one (key, value) pair.

**7 Geometry Object Types:**
 - “Point”
 - “MultiPoint” 
 - “LineString”
 - “MultiLineString”
 - “Polygon”
 - “MultiPolygon”
 - “GeometryCollection”

**2 Non-Geometry GeoJSON Object Types:**
 - “Feature”
 - “FeatureCollection”

**Key/Value of the Map**
- The key is QString containing one of the 9 GeoJson object types above described
- The value corresponding to this key has a different format depending on the type.

**Single Type Geometry Objects:**
- Key: “Point”
- Value: QGeoCircle

The value contains a QGeoCircle, the coordinates of the original GeoJSON point are stored in the center of this QGeoCircle

- Key: “LineString”
- Value: QGeoPath

The value contains a QGeoPath, the coordinates of the GeoJSON points included in the original LineString are stored in the path of the QGeoPath.

- Key: “Polygon”
- Value: QGeoPolygon

The value contains a QGeoPolygon, the coordinates of the GeoJSON points representing the outer perimeter are stored in the path of the QGeoPolygon, if holes are present, the coordinates are stored in the QGeoPolygon using the proper methods.

**Homogeneously Typed Multipart Geometry Objects:**
- Key: “MultiPoint”
- Value: QVariantList

The value contains a QVariantList, each element of this list is a QGeoCircle corresponding to a "Point".

- Key: “MultiLineString”
- Value: QVariantList

The value contains a QVariantList, each element of this list is QGeoPath corresponding to a "LineString".

- Key: “QVariantList”
- Value: QGeoCircle

The value contains a QVariantList, each element of this list is QGeoPolygon corresponding to a "Polygon".

**Heterogeneous composition of the other geometry types**
- Key: “GeometryCollection”
- Value: QGeoCircle

The value contains a QVariantList, each element of this list is a QVariantMap corresponding to one of the above listed GeoJson Geometry types, plus the GeometryCollection type itself.

**Objects including geometries and attributes for an entity**
- Key: “Feature”
- Value: QVariantMap

The value contains a QVariantMap. This map has a (key, value) pair corresponding the the pair of QVariantMap originated from one of the 7 GeoJSON geometry types already described. One more (key, value) pair conains a “properties” key and an additional QVariantMap as value. The (key, value) pairs of this last properties Map are pairs of QStrings.

- Key: “FeatureCollection”
- Value: QVariantList

The value contains a QVariantList, each element of this list is a QVariantMap corresponding to a "Feature", above described.

***Note:***
- GeoJson RFC advises against nesting GeometryCollections
- Validity of the parsed document can be queried with !isNull() or using external API's.
