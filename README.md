# qgeojson_class

The QGeoJson class provides a way to import and export a GeoJSON document to / from a custom QVariantMap.
This class can import and export this document in/from a QVariantMap build up like follows:

The class has 2 public methods:

QVariantMap importGeoJson(const QJsonDocument &geojsonDoc);
QJsonDocument exportGeoJson(const QVariantMap &geojsonMap);

The QvariantMap returned by the importer has a very specific architecture, shaped on the GeoJSON structure. 
The exporter accepts a QVariantMap with the same structures. This QVariantMap always has at least one (key, value) pair.

The key is QString containing one of the 9 GeoJson object types, which are:
7 Geometry Object Types: 
 - “Point”
 - “MultiPoint” 
 - “LineString”
 - “MultiLineString”
 - “Polygon”
 - “MultiPolygon”
 - “GeometryCollection”

2 Non-Geometry GeoJSON Object Types:
- “Feature”
- “FeatureCollection”

The value crresponding to this kye has a different format depending on the type:

Single Type Geometry Objects:
Key: “Point”
The value contains a QGeoCircle, The coordinates of the original GeoJSON point are stored in the center of this QGeoCircle

Key: “LineString”
The value contains a QGeoPath, The coordinates of the GeoJSON points included in the original LineString are stored in the path of the QGeoPath.

Key: “Polygon”
The value contains a QGeoPolygon, The coordinates of the GeoJSON points representing the outer perimeter are stored in the path of the QGeoPolygon, if holes are present, the coordinates are stored in the QGeoPolygon using the proper methods.

Homogeneously Typed Multipart Geometry Objects:
Key: “MultiPoint”
The value contains a QList<QGeoCircle>, The coordinates of the original GeoJSON points are stored in the center of these QGeoCircles

Key: “MultiLineString”
The value contains a QList<QGeoPath>, each structured like explained in the LineString section.

Key: “MultiPolygon”
The value contains a QList<QGeoPolygon>, each QgeoPolygon is structured like explained in the Polygon section.

Heterogeneous composition of the other geometry types
Key: “GeometryCollection”
The value contains a QVariantList, each element of this list is a QVariantMap corresponding to one of the above listed GeoJson Geometry types, plus the GeometryCollection type itself, albeit GeoJson RFC advises against nesting GeometryCollections.

Objects including geometries and attributes for an entity 
Key: “Feature”
The value contains a QVariantMap. 
This map has a (key, value) pair corresponding the the pair of QVariantMap originated from one of the 7 GeoJSON geometry types already described. One more (key, value) pair conains a “properties” key and an additional QVariantMap as value. 
The (key, value) pairs of this last properties Map are pairs of Qstrings

Key: “FeatureCollection”
The value contains a QList<QVariantMap>, each element of this list is a “Feature” type QVariantMap

A GeoJSON document can be converted from its text-based representation to a QJVariantMap using GeoJson::importGeoJson().

Validity of the parsed document can be queried with !isNull() or using external API's.
